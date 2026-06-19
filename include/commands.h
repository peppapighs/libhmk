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

#pragma once

#include "common.h"
#include "eeconfig.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// Commands
//--------------------------------------------------------------------+

typedef enum {
  COMMAND_FIRMWARE_VERSION = 0,
  COMMAND_REBOOT,
  COMMAND_BOOTLOADER,
  COMMAND_FACTORY_RESET,
  COMMAND_RECALIBRATE,
  COMMAND_ANALOG_INFO,
  COMMAND_GET_CALIBRATION,
  COMMAND_SET_CALIBRATION,
  COMMAND_GET_PROFILE,
  COMMAND_GET_OPTIONS,
  COMMAND_SET_OPTIONS,
  COMMAND_RESET_PROFILE,
  COMMAND_DUPLICATE_PROFILE,
  COMMAND_GET_METADATA,
  COMMAND_GET_SERIAL,
  COMMAND_SAVE_CALIBRATION_THRESHOLD,

  COMMAND_GET_KEYMAP = 128,
  COMMAND_SET_KEYMAP,
  COMMAND_GET_ACTUATION_MAP,
  COMMAND_SET_ACTUATION_MAP,
  COMMAND_GET_ADVANCED_KEYS,
  COMMAND_SET_ADVANCED_KEYS,
  COMMAND_GET_TICK_RATE,
  COMMAND_SET_TICK_RATE,
  COMMAND_GET_GAMEPAD_BUTTONS,
  COMMAND_SET_GAMEPAD_BUTTONS,
  COMMAND_GET_GAMEPAD_OPTIONS,
  COMMAND_SET_GAMEPAD_OPTIONS,
  COMMAND_GET_MACROS,
  COMMAND_SET_MACROS,

  COMMAND_UNKNOWN = 255,
} command_id_t;

//---------------------------------------------------------------------+
// Input Report Structures
//---------------------------------------------------------------------+

// Number of per-profile staged protocol data bytes that fit in a single raw HID
// packet after the command header fields.
#define COMMAND_SET_STAGED_PROFILE_BYTES_PER_PACKET 59
#define COMMAND_GET_STAGED_PROFILE_BYTES_PER_PACKET 62

typedef struct __attribute__((packed)) {
  uint8_t offset;
} command_in_analog_info_t;

typedef eeconfig_calibration_t command_in_calibration_t;

typedef eeconfig_options_t command_in_options_t;

typedef struct __attribute__((packed)) {
  uint8_t profile;
} command_in_reset_profile_t;

typedef struct __attribute__((packed)) {
  uint8_t profile;
  uint8_t src_profile;
} command_in_duplicate_profile_t;

typedef struct __attribute__((packed)) {
  uint32_t offset;
} command_in_metadata_t;

typedef struct __attribute__((packed)) {
  uint8_t profile;
  uint8_t layer;
  uint8_t offset;
  uint8_t len;
  uint8_t keymap[59];
} command_in_keymap_t;

typedef struct __attribute__((packed)) {
  uint8_t profile;
  uint8_t offset;
  uint8_t len;
  actuation_t actuation_map[15];
} command_in_actuation_map_t;

typedef struct __attribute__((packed)) {
  uint8_t profile;
  // Byte offset within the destination buffer.
  uint16_t offset;
  // Number of bytes to write from `data`
  uint8_t len;
  uint8_t data[COMMAND_SET_STAGED_PROFILE_BYTES_PER_PACKET];
} command_in_staged_profile_t;

typedef struct __attribute__((packed)) {
  uint8_t profile;
  uint8_t tick_rate;
} command_in_tick_rate_t;

typedef struct __attribute__((packed)) {
  uint8_t profile;
  uint8_t offset;
  uint8_t len;
  uint8_t gamepad_buttons[60];
} command_in_gamepad_buttons_t;

typedef struct __attribute__((packed)) {
  uint8_t profile;
  gamepad_options_t gamepad_options;
} command_in_gamepad_options_t;

// Command input buffer type
typedef struct __attribute__((packed)) {
  uint8_t command_id;
  union __attribute__((packed)) {
    command_in_analog_info_t analog_info;
    command_in_calibration_t calibration;
    command_in_options_t options;
    command_in_reset_profile_t reset_profile;
    command_in_duplicate_profile_t duplicate_profile;
    command_in_metadata_t metadata;

    command_in_keymap_t keymap;
    command_in_actuation_map_t actuation_map;
    command_in_tick_rate_t tick_rate;
    command_in_gamepad_buttons_t gamepad_buttons;
    command_in_gamepad_options_t gamepad_options;
    command_in_staged_profile_t staged_profile;
  };
} command_in_buffer_t;

_Static_assert(sizeof(command_in_buffer_t) <= RAW_HID_EP_SIZE,
               "Invalid command input buffer size");

//---------------------------------------------------------------------+
// Output Report Structures
//---------------------------------------------------------------------+

typedef struct __attribute__((packed)) {
  uint16_t adc_value;
  uint8_t distance;
} command_out_analog_info_t;

typedef struct __attribute__((packed)) {
  uint32_t len;
  uint8_t metadata[59];
} command_out_metadata_t;

typedef struct __attribute__((packed)) {
  // Number of valid bytes in `data`
  uint8_t len;
  uint8_t data[COMMAND_GET_STAGED_PROFILE_BYTES_PER_PACKET];
} command_out_staged_profile_t;

// Command output buffer type
typedef struct __attribute__((packed)) {
  uint8_t command_id;
  union __attribute__((packed)) {
    // For `COMMAND_FIRMWARE_VERSION`
    uint16_t firmware_version;
    // For `COMMAND_ANALOG_INFO`
    command_out_analog_info_t analog_info[21];
    // For `COMMAND_GET_CALIBRATION`
    eeconfig_calibration_t calibration;
    // For `COMMAND_GET_PROFILE`
    uint8_t current_profile;
    // For `COMMAND_GET_OPTIONS`
    eeconfig_options_t options;
    // For `COMMAND_GET_METADATA`
    command_out_metadata_t metadata;
    // For `COMMAND_GET_SERIAL`
    char serial[32];

    // For `COMMAND_GET_KEYMAP`
    uint8_t keymap[63];
    // For `COMMAND_GET_ACTUATION_MAP`
    actuation_t actuation_map[15];
    // For `COMMAND_GET_TICK_RATE`
    uint8_t tick_rate;
    // For `COMMAND_GET_GAMEPAD_BUTTONS`
    uint8_t gamepad_buttons[63];
    // For `COMMAND_GET_GAMEPAD_OPTIONS`
    gamepad_options_t gamepad_options;
    // For `COMMAND_GET_ADVANCED_KEYS` and `COMMAND_GET_MACROS`
    command_out_staged_profile_t staged_profile;
  };
} command_out_buffer_t;

_Static_assert(sizeof(command_out_buffer_t) <= RAW_HID_EP_SIZE,
               "Invalid command output buffer size");

//---------------------------------------------------------------------+
// Command API
//---------------------------------------------------------------------+

/**
 * @brief Initialize the command module
 *
 * @return None
 */
void command_init(void);

/**
 * @brief Queue a command buffer received from the raw HID interface
 *
 * Note that only one command can be queued at a time. The queued command will
 * be processed in the next call to `command_task`. Any subsequent commands
 * while a command is queued will be dropped.
 *
 * @param buf Command buffer
 * @param len Buffer length in bytes
 *
 * @return `true` if the command was queued
 */
bool command_enqueue(const uint8_t *buf, uint16_t len);

/**
 * @brief Process queued raw HID commands and send pending responses
 *
 * @return None
 */
void command_task(void);
