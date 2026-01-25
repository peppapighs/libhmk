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
// XInput Report
//--------------------------------------------------------------------+

#define XINPUT_SUBCLASS_DEFAULT 0x5D
#define XINPUT_PROTOCOL_DEFAULT 0x01
#define XINPUT_EP_SIZE 32
// Length of the XInput template descriptor: 39 bytes
#define XINPUT_DESC_LEN (9 + 16 + 7 + 7)

// XInput descriptor
#define XINPUT_DESCRIPTOR(itfnum, stridx, epout, epin, ep_interval)            \
  /* Interface */                                                              \
  9, TUSB_DESC_INTERFACE, itfnum, 0, 2, TUSB_CLASS_VENDOR_SPECIFIC,            \
      XINPUT_SUBCLASS_DEFAULT, XINPUT_PROTOCOL_DEFAULT, stridx,                \
                                                                               \
      /* Undocumented */                                                       \
      16, HID_DESC_TYPE_HID, U16_TO_U8S_LE(0x0110), 0x01, 0x24, epin, 0x14,    \
      0x03, 0x00, 0x03, 0x13, epout, 0x00, 0x03, 0x00,                         \
                                                                               \
      /* Endpoint In */                                                        \
      7, TUSB_DESC_ENDPOINT, epin, TUSB_XFER_INTERRUPT,                        \
      U16_TO_U8S_LE(XINPUT_EP_SIZE), ep_interval,                              \
                                                                               \
      /* Endpoint Out */                                                       \
      7, TUSB_DESC_ENDPOINT, epout, TUSB_XFER_INTERRUPT,                       \
      U16_TO_U8S_LE(XINPUT_EP_SIZE), ep_interval

// XInput buttons bitmask
typedef enum {
  XINPUT_BUTTON_UP = M_BIT(0),
  XINPUT_BUTTON_DOWN = M_BIT(1),
  XINPUT_BUTTON_LEFT = M_BIT(2),
  XINPUT_BUTTON_RIGHT = M_BIT(3),
  XINPUT_BUTTON_START = M_BIT(4),
  XINPUT_BUTTON_BACK = M_BIT(5),
  XINPUT_BUTTON_LS = M_BIT(6),
  XINPUT_BUTTON_RS = M_BIT(7),
  XINPUT_BUTTON_LB = M_BIT(8),
  XINPUT_BUTTON_RB = M_BIT(9),
  XINPUT_BUTTON_HOME = M_BIT(10),
  XINPUT_BUTTON_UNUSED = M_BIT(11),
  XINPUT_BUTTON_A = M_BIT(12),
  XINPUT_BUTTON_B = M_BIT(13),
  XINPUT_BUTTON_X = M_BIT(14),
  XINPUT_BUTTON_Y = M_BIT(15),
} xinput_buttons_bm_t;
typedef struct __attribute__((packed)) {
  uint8_t report_id;
  uint8_t report_size;
  uint16_t buttons;
  uint8_t lz;
  uint8_t rz;
  // lx, ly, rx, ry
  int16_t joysticks[4];
  uint8_t reserved[6];
} xinput_report_t;

_Static_assert(sizeof(xinput_report_t) == 20,
               "XInput report size must be 20 bytes");

//--------------------------------------------------------------------+
// XInput API
//--------------------------------------------------------------------+

/**
 * @brief Initialize the XInput module
 *
 * @return None
 */
void xinput_init(void);

/**
 * @brief Process an XInput key
 *
 * @param key Key index
 *
 * @return None
 */
void xinput_process(uint8_t key);

/**
 * @brief XInput task
 *
 * @return None
 */
void xinput_task(void);
