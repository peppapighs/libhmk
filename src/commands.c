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

#define COMMAND_STAGED_WRITE_BUFFER_SIZE                                       \
  M_MAX(sizeof(advanced_key_t), sizeof(macro_node_t) * NUM_MACRO_NODES)

typedef enum {
  COMMAND_STAGED_WRITE_NONE = 0,
  COMMAND_STAGED_WRITE_ADVANCED_KEY,
  COMMAND_STAGED_WRITE_MACROS,
} command_staged_write_type_t;

static struct {
  uint8_t type;
  uint8_t profile;
  uint32_t offset;
  uint8_t data[COMMAND_STAGED_WRITE_BUFFER_SIZE];
} staged_write_state;

static void command_reset_staged_write_state(void) {
  staged_write_state.type = COMMAND_STAGED_WRITE_NONE;
  staged_write_state.profile = 0;
  staged_write_state.offset = 0;
}

/**
 * @brief Write a fully staged configuration item
 *
 * @param profile Profile index
 * @param offset Byte offset within the staged field
 * @param data Staged data
 * @param len Length of the staged data
 *
 * @return `true` if the write was successful
 */
typedef bool (*command_staged_write_commit_t)(uint8_t profile, uint32_t offset,
                                              const uint8_t *data,
                                              uint32_t len);

/**
 * @brief Write the advanced key staged in `staged_write_state`
 *
 * Caller must make sure that the staged advanced key is complete.
 *
 * @return `true` if the write was successful
 */
static bool command_commit_staged_advanced_key(uint8_t profile, uint32_t offset,
                                               const uint8_t *data,
                                               uint32_t len) {
  if (len != sizeof(advanced_key_t) || offset % sizeof(advanced_key_t) != 0)
    return false;

  const uint8_t key_index = offset / sizeof(advanced_key_t);

  if (profile >= NUM_PROFILES || key_index >= NUM_ADVANCED_KEYS)
    return false;

  if (profile == eeconfig->current_profile)
    advanced_key_clear();

  const bool success = EECONFIG_WRITE_N(
      profiles[profile].advanced_keys[key_index], data, sizeof(advanced_key_t));

  if (profile == eeconfig->current_profile)
    layout_load_advanced_keys();

  return success;
}

/**
 * @brief Write the macro buffer staged in `staged_write_state`
 *
 * Caller must make sure that the staged macro buffer is complete.
 *
 * @return `true` if the write was successful
 */
static bool command_commit_staged_macros(uint8_t profile, uint32_t offset,
                                         const uint8_t *data, uint32_t len) {
  if (profile >= NUM_PROFILES)
    return false;

  const uint32_t macros_size = sizeof(eeconfig->profiles[profile].macros);

  if (len != macros_size || offset != 0)
    return false;

  if (profile == eeconfig->current_profile)
    advanced_key_clear();

  const uint32_t macros_offset = offsetof(eeconfig_t, profiles) +
                                 profile * sizeof(eeconfig_profile_t) +
                                 offsetof(eeconfig_profile_t, macros);
  return wear_leveling_write(macros_offset, data, len);
}

/**
 * @brief Stage a payload to be written only when a complete item is available
 *
 * @return `true` if the stage was successful
 */
static bool command_stage_write(uint8_t type, uint8_t profile, uint32_t offset,
                                const uint8_t *data, uint32_t len,
                                uint32_t field_size, uint32_t item_size,
                                uint32_t data_capacity,
                                command_staged_write_commit_t commit) {
  if (offset + len > field_size || len > data_capacity || len == 0 ||
      item_size > COMMAND_STAGED_WRITE_BUFFER_SIZE)
    goto fail;

  if (offset % item_size == 0) {
    // It is always safe to start staging at the beginning of an item.
    staged_write_state.type = type;
    staged_write_state.profile = profile;
    staged_write_state.offset = offset;
  }

  if (type != staged_write_state.type || offset != staged_write_state.offset ||
      profile != staged_write_state.profile)
    // Unexpected write type, offset, or profile mismatch.
    goto fail;

  for (uint32_t i = 0; i < len;) {
    const uint32_t current_item_offset = staged_write_state.offset % item_size;
    const uint32_t write_len = M_MIN(len - i, item_size - current_item_offset);

    memcpy(staged_write_state.data + current_item_offset, data + i, write_len);

    if (len - i >= item_size - current_item_offset) {
      const uint32_t item_offset =
          staged_write_state.offset - current_item_offset;
      const bool success =
          commit(profile, item_offset, staged_write_state.data, item_size);
      if (!success)
        goto fail;
    }

    staged_write_state.offset += write_len;
    i += write_len;
  }

  if (staged_write_state.offset % item_size == 0)
    command_reset_staged_write_state();

  return true;

fail:
  command_reset_staged_write_state();
  return false;
}

static bool
command_stage_advanced_key_write(const command_in_staged_profile_t *p) {
  return command_stage_write(
      COMMAND_STAGED_WRITE_ADVANCED_KEY, p->profile, p->offset, p->data, p->len,
      sizeof(eeconfig->profiles[p->profile].advanced_keys),
      sizeof(advanced_key_t), M_ARRAY_SIZE(p->data),
      command_commit_staged_advanced_key);
}

static bool command_stage_macro_write(const command_in_staged_profile_t *p) {
  const uint32_t macros_size = sizeof(eeconfig->profiles[p->profile].macros);

  return command_stage_write(COMMAND_STAGED_WRITE_MACROS, p->profile, p->offset,
                             p->data, p->len, macros_size, macros_size,
                             M_ARRAY_SIZE(p->data),
                             command_commit_staged_macros);
}

void command_init(void) {
  command_request_pending = false;
  command_response_pending = false;
  command_reset_staged_write_state();
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

  if (in->command_id != COMMAND_SET_ADVANCED_KEYS &&
      in->command_id != COMMAND_SET_MACROS)
    // Partial staged writes are only valid across consecutive staged write
    // packets.
    command_reset_staged_write_state();

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
    const command_in_staged_profile_t *p = &in->staged_profile;
    const uint32_t advanced_keys_size =
        sizeof(eeconfig->profiles[p->profile].advanced_keys);

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < advanced_keys_size);

    out->staged_profile.len = M_MIN(M_ARRAY_SIZE(out->staged_profile.data),
                                    advanced_keys_size - p->offset);
    memcpy(out->staged_profile.data,
           (const uint8_t *)eeconfig->profiles[p->profile].advanced_keys +
               p->offset,
           out->staged_profile.len);
    break;
  }
  case COMMAND_SET_ADVANCED_KEYS: {
    const command_in_staged_profile_t *p = &in->staged_profile;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);

    success = command_stage_advanced_key_write(p);
    break;
  }
  case COMMAND_GET_MACROS: {
    const command_in_staged_profile_t *p = &in->staged_profile;
    const uint32_t macros_size = sizeof(eeconfig->profiles[p->profile].macros);

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < macros_size);

    out->staged_profile.len =
        M_MIN(M_ARRAY_SIZE(out->staged_profile.data), macros_size - p->offset);
    memcpy(out->staged_profile.data,
           (const uint8_t *)eeconfig->profiles[p->profile].macros + p->offset,
           out->staged_profile.len);
    break;
  }
  case COMMAND_SET_MACROS: {
    const command_in_staged_profile_t *p = &in->staged_profile;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);

    success = command_stage_macro_write(p);
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