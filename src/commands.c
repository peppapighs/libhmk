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

#include "eeconfig.h"
#include "hardware/hardware.h"
#include "matrix.h"
#include "tusb.h"
#include "usb_descriptors.h"

static uint8_t out_buffer[RAW_HID_EP_SIZE];

void command_init(void) {}

void command_process(const uint8_t *buf) {
  const uint8_t command_id = buf[0];
  const uint8_t *command_data = buf + 1;

  bool success = true;
  switch (command_id) {
  case COMMAND_FIRMWARE_VERSION: {
    uint16_t *version = (uint16_t *)(out_buffer + 1);
    *version = FIRMWARE_VERSION;
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
    success = eeconfig_reset();
    break;
  }
  case COMMAND_RECALIBRATE: {
    matrix_recalibrate();
    break;
  }
  case COMMAND_ANALOG_INFO: {
    const uint8_t partial_size =
        COMMAND_PARTIAL_SIZE(sizeof(command_analog_info_t), 0);
    const uint8_t start = command_data[0] * partial_size;
    if (start >= NUM_KEYS) {
      // Invalid parameters
      success = false;
      break;
    }

    command_analog_info_t *res = (command_analog_info_t *)(out_buffer + 1);
    for (uint32_t i = 0; i < partial_size; i++) {
      if (start + i >= NUM_KEYS)
        break;

      res[i].adc_value = key_matrix[start + i].adc_filtered;
      res[i].distance = key_matrix[start + i].distance;
    }
    break;
  }
  case COMMAND_GET_CALIBRATION: {
    eeconfig_calibration_t *calibration =
        (eeconfig_calibration_t *)(out_buffer + 1);
    *calibration = eeconfig->calibration;
    break;
  }
  case COMMAND_SET_CALIBRATION: {
    success = eeconfig_set_calibration((const void *)command_data);
    break;
  }
  case COMMAND_GET_PROFILE: {
    uint8_t *profile = (uint8_t *)(out_buffer + 1);
    *profile = eeconfig->current_profile;
    break;
  }
  case COMMAND_GET_OPTIONS: {
    eeconfig_options_t *options = (eeconfig_options_t *)(out_buffer + 1);
    *options = eeconfig->options;
    break;
  }
  case COMMAND_SET_OPTIONS: {
    success = eeconfig_set_options((const void *)command_data);
    break;
  }
  case COMMAND_GET_KEYMAP: {
    const uint8_t partial_size = COMMAND_PARTIAL_SIZE(sizeof(uint8_t), 0);
    const uint8_t profile = command_data[0];
    const uint8_t layer = command_data[1];
    const uint8_t start = command_data[2] * partial_size;

    if (profile >= NUM_PROFILES || layer >= NUM_LAYERS || start >= NUM_KEYS) {
      // Invalid parameters
      success = false;
      break;
    }

    uint8_t *keymap = out_buffer + 1;
    for (uint32_t i = 0; i < partial_size; i++) {
      if (start + i >= NUM_KEYS)
        break;
      keymap[i] = eeconfig->profiles[profile].keymap[layer][start + i];
    }
    break;
  }
  case COMMAND_SET_KEYMAP: {
    const uint8_t partial_size = COMMAND_PARTIAL_SIZE(sizeof(uint8_t), 4);
    const uint8_t profile = command_data[0];
    const uint8_t layer = command_data[1];
    const uint8_t start = command_data[2];
    const uint8_t len = command_data[3];

    if (profile >= NUM_PROFILES || layer >= NUM_LAYERS ||
        start + len > NUM_KEYS || len > partial_size) {
      // Invalid parameters
      success = false;
      break;
    }

    success = eeconfig_set_keymap(profile, layer, start, len, command_data + 4);
    break;
  }
  case COMMAND_GET_ACTUATION_MAP: {
    const uint8_t partial_size = COMMAND_PARTIAL_SIZE(sizeof(actuation_t), 0);
    const uint8_t profile = command_data[0];
    const uint8_t start = command_data[1] * partial_size;

    if (profile >= NUM_PROFILES || start >= NUM_KEYS) {
      // Invalid parameters
      success = false;
      break;
    }

    actuation_t *actuation_map = (actuation_t *)(out_buffer + 1);
    for (uint32_t i = 0; i < partial_size; i++) {
      if (start + i >= NUM_KEYS)
        break;
      actuation_map[i] = eeconfig->profiles[profile].actuation_map[start + i];
    }
    break;
  }
  case COMMAND_SET_ACTUATION_MAP: {
    const uint8_t partial_size = COMMAND_PARTIAL_SIZE(sizeof(actuation_t), 3);
    const uint8_t profile = command_data[0];
    const uint8_t start = command_data[1];
    const uint8_t len = command_data[2];

    if (profile >= NUM_PROFILES || start + len > NUM_KEYS ||
        len > partial_size) {
      // Invalid parameters
      success = false;
      break;
    }

    success = eeconfig_set_actuation_map(profile, start, len, command_data + 3);
    break;
  }
  case COMMAND_GET_ADVANCED_KEYS: {
    const uint8_t partial_size =
        COMMAND_PARTIAL_SIZE(sizeof(advanced_key_t), 0);
    const uint8_t profile = command_data[0];
    const uint8_t start = command_data[1] * partial_size;

    if (profile >= NUM_PROFILES || start >= NUM_ADVANCED_KEYS) {
      // Invalid parameters
      success = false;
      break;
    }

    advanced_key_t *advanced_keys = (advanced_key_t *)(out_buffer + 1);
    for (uint32_t i = 0; i < partial_size; i++) {
      if (start + i >= NUM_ADVANCED_KEYS)
        break;
      advanced_keys[i] = eeconfig->profiles[profile].advanced_keys[start + i];
    }
    break;
  }
  case COMMAND_SET_ADVANCED_KEYS: {
    const uint8_t partial_size =
        COMMAND_PARTIAL_SIZE(sizeof(advanced_key_t), 3);
    const uint8_t profile = command_data[0];
    const uint8_t start = command_data[1];
    const uint8_t len = command_data[2];

    if (profile >= NUM_PROFILES || start + len > NUM_ADVANCED_KEYS ||
        len > partial_size) {
      // Invalid parameters
      success = false;
      break;
    }

    success = eeconfig_set_advanced_keys(profile, start, len, command_data + 3);
    break;
  }
  case COMMAND_GET_TICK_RATE: {
    const uint8_t profile = command_data[0];

    if (profile >= NUM_PROFILES) {
      // Invalid parameters
      success = false;
      break;
    }

    uint8_t *tick_rate = out_buffer + 1;
    *tick_rate = eeconfig->profiles[profile].tick_rate;
    break;
  }
  case COMMAND_SET_TICK_RATE: {
    const uint8_t profile = command_data[0];
    const uint8_t tick_rate = command_data[1];

    if (profile >= NUM_PROFILES) {
      // Invalid parameters
      success = false;
      break;
    }

    success = eeconfig_set_tick_rate(profile, tick_rate);
    break;
  }
  default: {
    // Unknown command
    success = false;
    break;
  }
  }

  // Echo the command ID back to the host if successful
  out_buffer[0] = success ? command_id : COMMAND_UNKNOWN;

  while (!tud_hid_n_ready(USB_ITF_RAW_HID))
    // Wait for the raw HID interface to be ready
    tud_task();
  tud_hid_n_report(USB_ITF_RAW_HID, 0, out_buffer, RAW_HID_EP_SIZE);
}
