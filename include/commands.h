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

  COMMAND_GET_KEYMAP = 128,
  COMMAND_SET_KEYMAP,
  COMMAND_GET_ACTUATION_MAP,
  COMMAND_SET_ACTUATION_MAP,
  COMMAND_GET_ADVANCED_KEYS,
  COMMAND_SET_ADVANCED_KEYS,
  COMMAND_GET_TICK_RATE,
  COMMAND_SET_TICK_RATE,

  COMMAND_UNKNOWN = 255,
} command_id_t;

//---------------------------------------------------------------------+
// Command Structures
//---------------------------------------------------------------------+

// Per key analog info
typedef struct __attribute__((packed)) {
  uint16_t adc_value;
  uint8_t distance;
} command_analog_info_t;

//---------------------------------------------------------------------+
// Command Macros
//---------------------------------------------------------------------+

// Number of entries that can be sent in a single command
#define COMMAND_PARTIAL_SIZE(size, header_size)                                \
  ((RAW_HID_EP_SIZE - 1 - header_size) / (size))

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
 * @brief Process a command buffer received from the raw HID interface
 *
 * @param buf Command buffer
 *
 * @return None
 */
void command_process(const uint8_t *buf);
