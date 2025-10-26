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

static uint8_t out_buf[RAW_HID_EP_SIZE];

void command_init(void) {}

void command_process(const uint8_t *buf) {
  const command_in_buffer_t *in = (const command_in_buffer_t *)buf;
  command_out_buffer_t *out = (command_out_buffer_t *)out_buf;

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

    COMMAND_VERIFY(p->offset < sizeof(metadata_compressed));

    out->metadata.len = sizeof(metadata_compressed) - p->offset;
    memcpy(out->metadata.metadata, &metadata_compressed[p->offset],
           M_MIN(sizeof(out->metadata.metadata), out->metadata.len));
    break;
  }
  case COMMAND_GET_SERIAL: {
    memset(out->serial, 0, sizeof(out->serial));
    board_serial(out->serial);
    break;
  }
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

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < NUM_ADVANCED_KEYS);

    memcpy(out->advanced_keys,
           eeconfig->profiles[p->profile].advanced_keys + p->offset,
           M_MIN(M_ARRAY_SIZE(out->advanced_keys),
                 (uint32_t)(NUM_ADVANCED_KEYS - p->offset)) *
               sizeof(advanced_key_t));
    break;
  }
  case COMMAND_SET_ADVANCED_KEYS: {
    const command_in_advanced_keys_t *p = &in->advanced_keys;

    COMMAND_VERIFY(p->profile < NUM_PROFILES);
    COMMAND_VERIFY(p->offset < NUM_ADVANCED_KEYS);
    COMMAND_VERIFY(p->len <= M_ARRAY_SIZE(p->advanced_keys) &&
                   p->len <= NUM_ADVANCED_KEYS - p->offset);

    if (p->profile == eeconfig->current_profile)
      advanced_key_clear();
    success =
        EECONFIG_WRITE_N(profiles[p->profile].advanced_keys[p->offset],
                         p->advanced_keys, sizeof(advanced_key_t) * p->len);
    if (p->profile == eeconfig->current_profile)
      layout_load_advanced_keys();
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

  while (!tud_hid_n_ready(USB_ITF_RAW_HID))
    // Wait for the raw HID interface to be ready
    tud_task();
  tud_hid_n_report(USB_ITF_RAW_HID, 0, out_buf, RAW_HID_EP_SIZE);
}
