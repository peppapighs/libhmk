/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "commands.h"

#include "advanced_keys.h"
#include "hardware/hardware.h"
#include "layout.h"
#include "matrix.h"
#include "metadata.h"
#include "tusb.h"

// Helper macro to verify command parameters
#define COMMAND_VERIFY(cond)                                                   \
  if (!(cond)) {                                                               \
    success = false;                                                           \
    break;                                                                     \
  }

static const uint8_t keyboard_metadata[] = {KEYBOARD_METADATA};

// `volatile` to prevent compiler optimizations
static volatile bool command_request_pending;
static volatile bool command_response_pending;
static uint8_t in_buf[RAW_HID_EP_SIZE];
static uint8_t out_buf[RAW_HID_EP_SIZE];

static struct {
  uint8_t profile;
  uint32_t offset;
  uint8_t data[sizeof(advanced_key_t)];
} advanced_key_write_state;

static void command_reset_advanced_key_write_state(void) {
  advanced_key_write_state.profile = 0;
  advanced_key_write_state.offset = 0;
}

/**
 * @brief Write the advanced key staged in `advanced_key_write_state`
 *
 * Caller must make sure that the staged advanced key is in a valid state.
 *
 * @return `true` if the write was successful
 */
static bool command_write_staged_advanced_key(void) {
  const uint8_t profile = advanced_key_write_state.profile;
  const uint8_t key_index =
      advanced_key_write_state.offset / sizeof(advanced_key_t);

  if (profile >= NUM_PROFILES || key_index >= NUM_ADVANCED_KEYS)
    return false;

  if (profile == eeconfig->current_profile)
    advanced_key_clear();

  const bool success =
      EECONFIG_WRITE_N(profiles[profile].advanced_keys[key_index],
                       advanced_key_write_state.data, sizeof(advanced_key_t));

  if (profile == eeconfig->current_profile)
    layout_load_advanced_keys();

  return success;
}

/**
 * @brief Stage the advanced key payload to be written at a later time to
 * prevent partial writes to the persistent configuration
 *
 * @return `true` if the stage was successful
 */
static bool
command_stage_advanced_key_write(const command_in_advanced_keys_t *p) {
  const uint32_t advanced_keys_size =
      sizeof(eeconfig->profiles[p->profile].advanced_keys);
  const uint32_t advanced_key_size = sizeof(advanced_key_t);

  if (p->offset + p->len > advanced_keys_size ||
      p->len > M_ARRAY_SIZE(p->data) || p->len == 0)
    goto fail;

  if (p->offset % advanced_key_size == 0) {
    // It is always safe to start staging at the beginning of an advanced key.
    advanced_key_write_state.profile = p->profile;
    advanced_key_write_state.offset = p->offset;
  }

  if (p->offset != advanced_key_write_state.offset ||
      p->profile != advanced_key_write_state.profile)
    // Unexpected write offset, or profile mismatch.
    goto fail;

  for (uint32_t i = 0; i < p->len;) {
    const uint32_t current_key_offset =
        advanced_key_write_state.offset % advanced_key_size;
    const uint32_t write_len =
        M_MIN(p->len - i, advanced_key_size - current_key_offset);

    memcpy(advanced_key_write_state.data + current_key_offset, p->data + i,
           write_len);

    if (p->len - i >= advanced_key_size - current_key_offset) {
      const bool success = command_write_staged_advanced_key();
      if (!success)
        goto fail;
    }

    advanced_key_write_state.offset += write_len;
    i += write_len;
  }

  return true;

fail:
  command_reset_advanced_key_write_state();
  return false;
}

void command_init(void) {
  command_request_pending = false;
  command_response_pending = false;
  command_reset_advanced_key_write_state();
}

bool command_enqueue(const uint8_t *buf, uint16_t len) {
  if (len != RAW_HID_EP_SIZE || command_request_pending ||
      command_response_pending)
    // Either `command_request_pending` or `command_response_pending` is set
    // means that there is already a command queued.
    return false;

  memcpy(in_buf, buf, RAW_HID_EP_SIZE);
  command_request_pending = true;

  return true;
}

/**
 * @brief Process the queued command and write the response
 *
 * @return None
 */
static void command_process(void) {
  const command_in_buffer_t *in = (const command_in_buffer_t *)in_buf;
  command_out_buffer_t *out = (command_out_buffer_t *)out_buf;

  if (in->command_id != COMMAND_SET_ADVANCED_KEYS)
    // Partial advanced key writes are only valid across consecutive
    // `COMMAND_SET_ADVANCED_KEYS` packets.
    command_reset_advanced_key_write_state();

  bool success = true;
  switch (in->command_id) {
  case COMMAND_FIRMWARE_VERSION: {
    out->firmware_version = FIRMWARE_VERSION;
    break;
  }
  case COMMAND_REBOOT: {
    board_reset();
    break;
  }
  case COMMAND_BOOTLOADER: {
    board_enter_bootloader();
    break;
  }
  case COMMAND_FACTORY_RESET: {
    advanced_key_clear();
    success = eeconfig_reset();
    layout_load_advanced_keys();
    break;
  }
  case COMMAND_RECALIBRATE: {
    matrix_recalibrate(true);
    break;
  }
  case COMMAND_ANALOG_INFO: {
    const command_in_analog_info_t *p = &in->analog_info;
    command_out_analog_info_t *o = out->analog_info;

    COMMAND_VERIFY(p->offset < NUM_KEYS);

    for (uint32_t i = 0;
         i < M_ARRAY_SIZE(out->analog_info) && i + p->offset < NUM_KEYS; i++) {
      o[i].adc_value = key_matrix[i + p->offset].adc_filtered;
      o[i].distance = key_matrix[i + p->offset].distance;
    }
    break;
  }
  case COMMAND_GET_CALIBRATION: {
    out->calibration = eeconfig->calibration;
    break;
  }
  case COMMAND_SET_CALIBRATION: {
    success = EECONFIG_WRITE(calibration, &in->calibration);
    break;
  }
  case COMMAND_GET_PROFILE: {
    out->current_profile = eeconfig->current_profile;
    break;
  }
  case COMMAND_GET_OPTIONS: {
    out->options = eeconfig->options;
    break;
  }
  case COMMAND_SET_OPTIONS: {
    success = EECONFIG_WRITE(options, &in->options);
    break;
  }
  case COMMAND_RESET_PROFILE: {
    const command_in_reset_profile_t *p = &in->reset_profile;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);

    if (p->profile == eeconfig->current_profile)
      advanced_key_clear();
    success = eeconfig_reset_profile(p->profile);
    if (p->profile == eeconfig->current_profile)
      layout_load_advanced_keys();
    break;
  }
  case COMMAND_DUPLICATE_PROFILE: {
    const command_in_duplicate_profile_t *p = &in->duplicate_profile;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->src_profile < NUM_PROFILES);

    if (p->profile == eeconfig->current_profile)
      advanced_key_clear();
    success = EECONFIG_WRITE(profiles[p->profile],
                             &eeconfig->profiles[p->src_profile]);
    if (p->profile == eeconfig->current_profile)
      layout_load_advanced_keys();
    break;
  }
  case COMMAND_GET_KEYMAP: {
    const command_in_keymap_t *p = &in->keymap;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->layer < NUM_LAYERS);
    COMMAND_VERIFY(p->offset < NUM_KEYS);

    memcpy(out->keymap,
           eeconfig->profiles[p->profile].keymap[p->layer] + p->offset,
           M_MIN(M_ARRAY_SIZE(out->keymap), (uint32_t)(NUM_KEYS - p->offset)) *
               sizeof(uint8_t));
    break;
  }
  case COMMAND_GET_METADATA: {
    const command_in_metadata_t *p = &in->metadata;

    COMMAND_VERIFY(p->offset < sizeof(keyboard_metadata));

    out->metadata.len = sizeof(keyboard_metadata) - p->offset;
    memcpy(out->metadata.metadata, &keyboard_metadata[p->offset],
           M_MIN(sizeof(out->metadata.metadata), out->metadata.len));
    break;
  }
  case COMMAND_GET_SERIAL: {
    memset(out->serial, 0, sizeof(out->serial));
    board_serial(out->serial);
    break;
  }
  case COMMAND_SAVE_CALIBRATION_THRESHOLD: {
    uint16_t bottom_out_threshold[NUM_KEYS];

    for (uint32_t i = 0; i < NUM_KEYS; i++) {
      if (key_matrix[i].adc_bottom_out_value < key_matrix[i].adc_rest_value)
        bottom_out_threshold[i] = 0;
      else
        bottom_out_threshold[i] =
            key_matrix[i].adc_bottom_out_value - key_matrix[i].adc_rest_value;
    }
    success = EECONFIG_WRITE(bottom_out_threshold, bottom_out_threshold);
    break;
  }
    //--------------------------------------------------------------------+
    // Per-profile commands
    //--------------------------------------------------------------------+
  case COMMAND_SET_KEYMAP: {
    const command_in_keymap_t *p = &in->keymap;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->layer < NUM_LAYERS);
    COMMAND_VERIFY(p->offset < NUM_KEYS);
    COMMAND_VERIFY(p->len <= M_ARRAY_SIZE(p->keymap) &&
                   p->len <= NUM_KEYS - p->offset);

    success = EECONFIG_WRITE_N(profiles[p->profile].keymap[p->layer][p->offset],
                               p->keymap, sizeof(uint8_t) * p->len);
    break;
  }
  case COMMAND_GET_ACTUATION_MAP: {
    const command_in_actuation_map_t *p = &in->actuation_map;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < NUM_KEYS);

    memcpy(out->actuation_map,
           eeconfig->profiles[p->profile].actuation_map + p->offset,
           M_MIN(M_ARRAY_SIZE(out->actuation_map),
                 (uint32_t)(NUM_KEYS - p->offset)) *
               sizeof(actuation_t));
    break;
  }
  case COMMAND_SET_ACTUATION_MAP: {
    const command_in_actuation_map_t *p = &in->actuation_map;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < NUM_KEYS);
    COMMAND_VERIFY(p->len <= M_ARRAY_SIZE(p->actuation_map) &&
                   p->len <= NUM_KEYS - p->offset);

    success = EECONFIG_WRITE_N(profiles[p->profile].actuation_map[p->offset],
                               p->actuation_map, sizeof(actuation_t) * p->len);
    break;
  }
  case COMMAND_GET_ADVANCED_KEYS: {
    const command_in_advanced_keys_t *p = &in->advanced_keys;
    const uint32_t advanced_keys_size =
        sizeof(eeconfig->profiles[p->profile].advanced_keys);

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < advanced_keys_size);

    out->advanced_keys.len = M_MIN(M_ARRAY_SIZE(out->advanced_keys.data),
                                   advanced_keys_size - p->offset);
    memcpy(out->advanced_keys.data,
           (const uint8_t *)eeconfig->profiles[p->profile].advanced_keys +
               p->offset,
           out->advanced_keys.len);
    break;
  }
  case COMMAND_SET_ADVANCED_KEYS: {
    const command_in_advanced_keys_t *p = &in->advanced_keys;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);

    success = command_stage_advanced_key_write(p);
    break;
  }
  case COMMAND_GET_STRING_MACROS: {
    const command_in_string_macros_t *p = &in->string_macros;
    const uint32_t string_macros_size =
        sizeof(eeconfig->profiles[p->profile].string_macros);

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < string_macros_size);

    out->string_macros.len = M_MIN(M_ARRAY_SIZE(out->string_macros.data),
                                   string_macros_size - p->offset);
    memcpy(out->string_macros.data,
           eeconfig->profiles[p->profile].string_macros + p->offset,
           out->string_macros.len);
    break;
  }
  case COMMAND_SET_STRING_MACROS: {
    const command_in_string_macros_t *p = &in->string_macros;
    const uint32_t string_macros_size =
        sizeof(eeconfig->profiles[p->profile].string_macros);

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < string_macros_size);
    COMMAND_VERIFY(p->len <= M_ARRAY_SIZE(p->data) &&
                   p->len <= string_macros_size - p->offset);

    if (p->profile == eeconfig->current_profile)
      advanced_key_clear();
    success = EECONFIG_WRITE_N(profiles[p->profile].string_macros[p->offset],
                               p->data, sizeof(uint8_t) * p->len);
    break;
  }
  case COMMAND_GET_TICK_RATE: {
    const command_in_tick_rate_t *p = &in->tick_rate;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);

    out->tick_rate = eeconfig->profiles[p->profile].tick_rate;
    break;
  }
  case COMMAND_SET_TICK_RATE: {
    const command_in_tick_rate_t *p = &in->tick_rate;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);

    success = EECONFIG_WRITE(profiles[p->profile].tick_rate, &p->tick_rate);
    break;
  }
  case COMMAND_GET_GAMEPAD_BUTTONS: {
    const command_in_gamepad_buttons_t *p = &in->gamepad_buttons;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < NUM_KEYS);

    memcpy(out->gamepad_buttons,
           eeconfig->profiles[p->profile].gamepad_buttons + p->offset,
           M_MIN(M_ARRAY_SIZE(out->gamepad_buttons),
                 (uint32_t)(NUM_KEYS - p->offset)) *
               sizeof(uint8_t));
    break;
  }
  case COMMAND_SET_GAMEPAD_BUTTONS: {
    const command_in_gamepad_buttons_t *p = &in->gamepad_buttons;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < NUM_KEYS);
    COMMAND_VERIFY(p->len <= M_ARRAY_SIZE(p->gamepad_buttons) &&
                   p->len <= NUM_KEYS - p->offset);

    success = EECONFIG_WRITE_N(profiles[p->profile].gamepad_buttons[p->offset],
                               p->gamepad_buttons, sizeof(uint8_t) * p->len);
    break;
  }
  case COMMAND_GET_GAMEPAD_OPTIONS: {
    const command_in_gamepad_options_t *p = &in->gamepad_options;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);

    out->gamepad_options = eeconfig->profiles[p->profile].gamepad_options;
    break;
  }
  case COMMAND_SET_GAMEPAD_OPTIONS: {
    const command_in_gamepad_options_t *p = &in->gamepad_options;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);

    success = EECONFIG_WRITE(profiles[p->profile].gamepad_options,
                             &p->gamepad_options);
    break;
  }
  default: {
    // Unknown command
    success = false;
    break;
  }
  }

  // Echo the command ID back to the host if successful
  out->command_id = success ? in->command_id : COMMAND_UNKNOWN;
}

void command_task(void) {
  if (command_request_pending) {
    command_process();
    command_request_pending = false;
    command_response_pending = true;
  }

  if (command_response_pending && tud_hid_n_ready(USB_ITF_RAW_HID) &&
      tud_hid_n_report(USB_ITF_RAW_HID, 0, out_buf, RAW_HID_EP_SIZE))
    // The command response has been sent, so clear the queue.
    command_response_pending = false;
}
