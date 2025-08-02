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

#define XINPUT_EP_SIZE 32
// Length of the XInput template descriptor: 40 bytes
#define XINPUT_DESC_LEN (9 + 17 + 7 + 7)

// XInput descriptor
#define XINPUT_DESCRIPTOR(itfnum, stridx, epout, epin)                         \
  /* Interface */                                                              \
  9, TUSB_DESC_INTERFACE, itfnum, 0, 2, TUSB_CLASS_VENDOR_SPECIFIC, 0x5D,      \
      0x01, stridx,                                                            \
                                                                               \
      /* Undocumented */                                                       \
      17, HID_DESC_TYPE_HID, U16_TO_U8S_LE(0x0100), 0x01, 0x25, epin, 0x14,    \
      0x00, 0x00, 0x00, 0x00, 0x13, epout, 0x08, 0x00, 0x00,                   \
                                                                               \
      /* Endpoint In */                                                        \
      7, TUSB_DESC_ENDPOINT, epin, TUSB_XFER_INTERRUPT,                        \
      U16_TO_U8S_LE(XINPUT_EP_SIZE), 0x04,                                     \
                                                                               \
      /* Endpoint Out */                                                       \
      7, TUSB_DESC_ENDPOINT, epout, TUSB_XFER_INTERRUPT,                       \
      U16_TO_U8S_LE(XINPUT_EP_SIZE), 0x08

typedef enum {
  XINPUT_BUTTON_UP = M_BIT(0),
  XINPUT_BUTTON_DOWN = M_BIT(1),
  XINPUT_BUTTON_LEFT = M_BIT(2),
  XINPUT_BUTTON_RIGHT = M_BIT(3),
  XINPUT_BUTTON_START = M_BIT(4),
  XINPUT_BUTTON_BACK = M_BIT(5),
  XINPUT_BUTTON_LS = M_BIT(6),
  XINPUT_BUTTON_RS = M_BIT(7),
} xinput_buttons_0_bm_t;

typedef enum {
  XINPUT_BUTTON_LB = M_BIT(0),
  XINPUT_BUTTON_RB = M_BIT(1),
  XINPUT_BUTTON_HOME = M_BIT(2),
  // Unused M_BIT(3)
  XINPUT_BUTTON_A = M_BIT(4),
  XINPUT_BUTTON_B = M_BIT(5),
  XINPUT_BUTTON_X = M_BIT(6),
  XINPUT_BUTTON_Y = M_BIT(7),
} xinput_buttons_1_bm_t;

typedef struct __attribute__((packed)) {
  uint8_t report_id;
  // Must always be 20 bytes
  uint8_t report_size;
  uint8_t buttons_0;
  uint8_t buttons_1;
  uint8_t lz;
  uint8_t rz;
  int16_t lx;
  int16_t ly;
  int16_t rx;
  int16_t ry;
  uint8_t reserved[6];
} xinput_report_t;

//--------------------------------------------------------------------+
// XInput API
//--------------------------------------------------------------------+

/**
 * @brief Send an XInput report
 *
 * @param report Pointer to the XInput report to send
 *
 * @return true if the report was sent successfully, false otherwise
 */
bool xinput_send_report(const xinput_report_t *report);
