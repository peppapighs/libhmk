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
#if defined(RGB_ENABLE)
#include "rgb.h"
#endif
#include "tusb.h"

// Helper macro to verify command parameters
#define COMMAND_VERIFY(cond)                                                   \
  if (!(cond)) {                                                               \
    success = false;                                                           \
    break;                                                                     \
  }

static const uint8_t keyboard_metadata[] = {KEYBOARD_METADATA};

static bool gamepad_options_are_valid(const gamepad_options_t *options) {
  if (options == NULL)
    return false;
  for (uint8_t i = 0u; i < 3u; i++) {
    if (options->analog_curve[i][0] >= options->analog_curve[i + 1u][0])
      return false;
  }
  return true;
}

static bool gamepad_buttons_are_valid(const uint8_t *buttons, uint8_t len) {
  if (buttons == NULL)
    return false;
  for (uint8_t i = 0u; i < len; i++) {
    if (buttons[i] > GP_BUTTON_RT)
      return false;
  }
  return true;
}

// `volatile` to prevent compiler optimizations
static volatile bool command_request_pending;
static volatile bool command_response_pending;
static uint8_t in_buf[RAW_HID_EP_SIZE];
static uint8_t out_buf[RAW_HID_EP_SIZE];

command_staged_buffer_t staged_buffer;

static void command_reset_staged_buffer(void) {
  staged_buffer.staged_id = COMMAND_STAGED_NONE;
  staged_buffer.profile = 0;
  staged_buffer.offset = 0;
}

/**
 * @brief Write the advanced key staged in `staged_buffer`
 *
 * Caller must make sure that the staged advanced key is in a valid state.
 *
 * @return `true` if the write was successful
 */
static bool command_write_staged_advanced_key(void) {
  if (staged_buffer.staged_id != COMMAND_STAGED_ADVANCED_KEYS)
    return false;

  const uint8_t profile = staged_buffer.profile;
  const uint8_t key_index = staged_buffer.offset / sizeof(advanced_key_t);

  if (profile >= NUM_PROFILES || key_index >= NUM_ADVANCED_KEYS)
    return false;

  if (profile == eeconfig->current_profile)
    advanced_key_clear();

  const bool success = EECONFIG_WRITE_N(
      profiles[profile].advanced_keys[key_index],
      &staged_buffer.data.advanced_key, sizeof(advanced_key_t));

  if (profile == eeconfig->current_profile)
    layout_load_advanced_keys();

  return success;
}

/**
 * @brief Write the macro node staged in `staged_buffer`
 *
 * Caller must make sure that the staged macro node is in a valid state.
 *
 * @return `true` if the write was successful
 */
static bool command_write_staged_macro(void) {
  if (staged_buffer.staged_id != COMMAND_STAGED_MACROS)
    return false;

  const uint8_t profile = staged_buffer.profile;
  const uint8_t node_id = staged_buffer.offset / sizeof(macro_node_t);

  if (profile >= NUM_PROFILES || node_id >= NUM_MACRO_NODES)
    return false;

  if (profile == eeconfig->current_profile)
    advanced_key_clear();

  const bool success =
      EECONFIG_WRITE_N(profiles[profile].macros[node_id],
                       &staged_buffer.data.macro_node, sizeof(macro_node_t));

  if (profile == eeconfig->current_profile)
    layout_load_advanced_keys();

  return success;
}

/**
 * @brief Stage the staged protocol payload to be written at a later time to
 * prevent partial writes to the persistent configuration
 *
 * @return `true` if the stage was successful
 */
__attribute__((always_inline)) static inline bool
command_stage_write(const command_staged_write_t args) {
  const uint8_t staged_id = args.staged_id;
  const command_in_staged_profile_t *p = args.p;
  const uint32_t field_size = args.field_size;
  const uint32_t item_size = args.item_size;
  bool (*write_func)(void) = args.write_func;

  if (p->offset + p->len > field_size || p->len > M_ARRAY_SIZE(p->data) ||
      p->len == 0)
    goto fail;

  if (p->offset % item_size == 0) {
    // It is always safe to start staging at the beginning of an item.
    staged_buffer.staged_id = staged_id;
    staged_buffer.profile = p->profile;
    staged_buffer.offset = p->offset;
  }

  if (staged_id != staged_buffer.staged_id ||
      p->offset != staged_buffer.offset || p->profile != staged_buffer.profile)
    // Unexpected staged id, write offset, or profile mismatch.
    goto fail;

  for (uint32_t i = 0; i < p->len;) {
    const uint32_t current_item_offset = staged_buffer.offset % item_size;
    const uint32_t write_len =
        M_MIN(p->len - i, item_size - current_item_offset);

    memcpy(staged_buffer.raw_data + current_item_offset, p->data + i,
           write_len);

    if (p->len - i >= item_size - current_item_offset) {
      const bool success = write_func();
      if (!success)
        goto fail;
    }

    staged_buffer.offset += write_len;
    i += write_len;
  }

  return true;

fail:
  command_reset_staged_buffer();
  return false;
}

void command_init(void) {
  command_request_pending = false;
  command_response_pending = false;
  command_reset_staged_buffer();
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

  /* Responses are fixed-size RAW HID reports. Clear unused bytes so a short
   * response cannot leak payload left behind by an earlier command. */
  memset(out_buf, 0, sizeof(out_buf));

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
    eeconfig_options_t options = in->options;
    COMMAND_VERIFY(!(options.xinput_enabled && options.hid_gamepad_enabled));
    success = EECONFIG_WRITE(options, &options);
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
#if defined(RGB_ENABLE)
  case COMMAND_GET_LED_ENABLED: {
    out_buf[1] = 0;
    out_buf[2] = rgb_is_enabled() ? 1u : 0u;
    break;
  }
  case COMMAND_SET_LED_ENABLED: {
    if (in_buf[2] > 1u)
      out_buf[1] = 3u;
    else
      out_buf[1] = rgb_set_enabled(in_buf[2] != 0u, true) ? 0u : 1u;
    out_buf[2] = rgb_is_enabled() ? 1u : 0u;
    break;
  }
  case COMMAND_GET_LED_BRIGHTNESS: {
    out_buf[1] = 0;
    out_buf[2] = rgb_get_brightness();
    break;
  }
  case COMMAND_SET_LED_BRIGHTNESS: {
    out_buf[1] = rgb_set_brightness(in_buf[2], true) ? 0u : 1u;
    out_buf[2] = rgb_get_brightness();
    break;
  }
  case COMMAND_GET_LED_PIXEL: {
    uint8_t r = 0, g = 0, b = 0;
    out_buf[2] = in_buf[2];
    out_buf[1] = rgb_get_pixel(in_buf[2], &r, &g, &b) ? 0u : 3u;
    out_buf[3] = r;
    out_buf[4] = g;
    out_buf[5] = b;
    break;
  }
  case COMMAND_SET_LED_PIXEL: {
    out_buf[2] = in_buf[2];
    out_buf[3] = in_buf[3];
    out_buf[4] = in_buf[4];
    out_buf[5] = in_buf[5];
    out_buf[1] = rgb_set_pixel(in_buf[2], in_buf[3], in_buf[4], in_buf[5])
                     ? 0u
                     : 3u;
    break;
  }
  case COMMAND_GET_LED_ALL: {
    const uint8_t chunk = in_buf[2];
    const uint16_t offset = (uint16_t)chunk * 60u;
    const uint16_t remaining = offset < (uint16_t)RGB_FRAME_BYTES
                                   ? (uint16_t)((uint16_t)RGB_FRAME_BYTES -
                                                offset)
                                   : 0u;
    const uint8_t len = remaining > 60u ? 60u : (uint8_t)remaining;
    out_buf[2] = chunk;
    out_buf[3] = len;
    out_buf[1] = rgb_get_frame_chunk(offset, out_buf + 4, len) ? 0u : 3u;
    break;
  }
  case COMMAND_SET_LED_ALL_CHUNK: {
    const uint8_t chunk = in_buf[2];
    const uint8_t len = in_buf[3];
    const uint16_t offset = (uint16_t)chunk * 60u;
    out_buf[2] = chunk;
    out_buf[3] = len;
    out_buf[1] = (len <= 60u && rgb_set_frame_chunk(offset, in_buf + 4, len))
                     ? 0u
                     : 3u;
    break;
  }
  case COMMAND_LED_CLEAR: {
    out_buf[1] = rgb_fill(0, 0, 0, rgb_get_effect() != RGB_EFFECT_LIVE)
                     ? 0u
                     : 1u;
    break;
  }
  case COMMAND_LED_FILL: {
    out_buf[1] = rgb_fill(in_buf[2], in_buf[3], in_buf[4],
                          rgb_get_effect() != RGB_EFFECT_LIVE)
                     ? 0u
                     : 1u;
    out_buf[2] = in_buf[2];
    out_buf[3] = in_buf[3];
    out_buf[4] = in_buf[4];
    break;
  }
  case COMMAND_GET_LED_EFFECT: {
    out_buf[1] = 0;
    out_buf[2] = rgb_get_effect();
    break;
  }
  case COMMAND_SET_LED_EFFECT: {
    const uint8_t effect = in_buf[2];
    const bool valid = effect == RGB_EFFECT_STATIC ||
                       effect == RGB_EFFECT_BREATHING ||
                       effect == RGB_EFFECT_RAINBOW ||
                       effect == RGB_EFFECT_RAINBOW_WAVE ||
                       effect == RGB_EFFECT_LIVE;
    if (!valid)
      out_buf[1] = 3u;
    else
      out_buf[1] = rgb_set_effect(effect, effect != RGB_EFFECT_LIVE) ? 0u : 1u;
    out_buf[2] = rgb_get_effect();
    break;
  }
  case COMMAND_RESTORE_LED_EFFECT: {
    out_buf[1] = rgb_restore_effect() ? 0u : 1u;
    out_buf[2] = rgb_get_effect();
    break;
  }
  case COMMAND_GET_RGB_CAPABILITIES: {
    out_buf[1] = 0;
    out_buf[2] = 1; /* protocol major */
    out_buf[3] = 0; /* protocol minor */
    out_buf[4] = RGB_LED_COUNT;
    out_buf[5] = RGB_BYTES_PER_PIXEL;
    out_buf[6] = 60;
    out_buf[7] = RGB_EFFECT_LIVE;
    out_buf[8] = 0x7f; /* enabled..restore */
    out_buf[9] = 0;
    out_buf[10] = 0; /* logical RGB */
    break;
  }
#endif
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

    success = command_stage_write((command_staged_write_t){
        .staged_id = COMMAND_STAGED_ADVANCED_KEYS,
        .p = (command_in_staged_profile_t *)p,
        .field_size = sizeof(eeconfig->profiles[p->profile].advanced_keys),
        .item_size = sizeof(advanced_key_t),
        .write_func = command_write_staged_advanced_key,
    });
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

    success = command_stage_write((command_staged_write_t){
        .staged_id = COMMAND_STAGED_MACROS,
        .p = (command_in_staged_profile_t *)p,
        .field_size = sizeof(eeconfig->profiles[p->profile].macros),
        .item_size = sizeof(macro_node_t),
        .write_func = command_write_staged_macro,
    });
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
    COMMAND_VERIFY(gamepad_buttons_are_valid(p->gamepad_buttons, p->len));

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
    COMMAND_VERIFY(gamepad_options_are_valid(&p->gamepad_options));

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
