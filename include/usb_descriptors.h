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

#include "class/hid/hid.h"
#include "common.h"

//--------------------------------------------------------------------+
// USB Configuration
//--------------------------------------------------------------------+

#if !defined(USB_MANUFACTURER_NAME)
#define USB_MANUFACTURER_NAME "Unknown"
#endif

#if !defined(USB_PRODUCT_NAME)
#define USB_PRODUCT_NAME "Keyboard"
#endif

#if !defined(USB_VENDOR_ID)
#define USB_VENDOR_ID 0xCAFE
#endif

#if !defined(USB_PRODUCT_ID)
#define USB_PRODUCT_ID 0x1234
#endif

//--------------------------------------------------------------------+
// USB Descriptors
//--------------------------------------------------------------------+

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
#if defined(LOG_ENABLED)
  USB_ITF_LOG,
#endif
  USB_ITF_COUNT,
};

enum {
  REPORT_ID_SYSTEM_CONTROL = 1,
  REPORT_ID_CONSUMER_CONTROL,
  REPORT_ID_MOUSE,
  REPORT_ID_COUNT,
};

//--------------------------------------------------------------------+
// NKRO HID Report
//--------------------------------------------------------------------+

// 20 * 8 = 160 bits for HID keyboard keycodes up to KC_LANGUAGE_2
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

//--------------------------------------------------------------------+
// Log HID Report
//--------------------------------------------------------------------+

#if defined(LOG_ENABLED)
#define LOG_EP_SIZE 32
// Vendor defined usage page (PJRC Teensy compatible)
#define LOG_USAGE_PAGE 0xFF31
// Vendor defined usage ID (PJRC Teensy compatible)
#define LOG_USAGE 0x74
#endif
