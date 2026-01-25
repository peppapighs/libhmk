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
#include "tusb_config.h"

//--------------------------------------------------------------------+
// USB Descriptors
//--------------------------------------------------------------------+

#define USB_BCD_VERSION 0x0210

enum {
  STR_ID_LANGID = 0,
  STR_ID_MANUFACTURER,
  STR_ID_PRODUCT,
  STR_ID_SERIAL,
  STR_ID_COUNT,
};

enum {
  // Separate interface for keyboard to support boot protocol
  USB_ITF_KEYBOARD = 0,
  USB_ITF_HID,
  USB_ITF_RAW_HID,
  // We intentionally put the XInput interface last, so that if it is not
  // enabled, we can subtract its size from the total configuration length
  // without affecting the other interfaces.
  USB_ITF_XINPUT,
  USB_ITF_COUNT,
};

// In endpoint addresses
enum {
  EP_IN_ADDR_KEYBOARD = 0x81,
  EP_IN_ADDR_HID,
  EP_IN_ADDR_RAW_HID,
  EP_IN_ADDR_XINPUT,
};

// Out endpoint addresses
enum {
  EP_OUT_ADDR_RAW_HID = 0x01,
  EP_OUT_ADDR_XINPUT,
};

enum {
  REPORT_ID_SYSTEM_CONTROL = 1,
  REPORT_ID_CONSUMER_CONTROL,
  REPORT_ID_MOUSE,
  REPORT_ID_COUNT,
};

//---------------------------------------------------------------------+
// Microsoft OS 2.0 descriptor
//---------------------------------------------------------------------+

// Microsoft OS 2.0 descriptor vendor code
#define MS_OS_20_VENDOR_CODE 0x01
// Microsoft OS 2.0 descriptor length
#define MS_OS_20_DESC_LEN 178

//--------------------------------------------------------------------+
// NKRO HID Report
//--------------------------------------------------------------------+

// 20 * 8 = 160 bits for HID keyboard keycodes up to KC_LANGUAGE_6
#define NUM_NKRO_BYTES 20

// NKRO report with 6-KRO fallback
// https://geekhack.org/index.php?topic=13162
// https://www.devever.net/~hl/usbnkro.
typedef struct __attribute__((packed)) {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keycodes[6];
  uint8_t bitmap[NUM_NKRO_BYTES];
} hid_nkro_kb_report_t;

// Strictly larger since this report belongs to an interface with multiple
// reports, so the first byte is reserved for the report ID
_Static_assert(sizeof(hid_nkro_kb_report_t) < CFG_TUD_HID_EP_BUFSIZE,
               "Invalid NKRO report size");

//--------------------------------------------------------------------+
// Raw HID Report
//--------------------------------------------------------------------+

#define RAW_HID_EP_SIZE 64
// Vendor defined usage page
#define RAW_HID_USAGE_PAGE 0xFFAB
// Vendor defined usage ID
#define RAW_HID_USAGE 0xAB

// Otherwise, SET_REPORT callback will only be called when TinyUSB endpoint
// buffer is full, resulting in missed reports.
_Static_assert(RAW_HID_EP_SIZE == CFG_TUD_HID_EP_BUFSIZE,
               "Invalid Raw HID report size");
