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

#if !defined(CFG_TUSB_MCU)
#error "CFG_TUSB_MCU is not defined"
#endif

// Default TinyUSB configuration
#define CFG_TUSB_OS OPT_OS_NONE
#define CFG_TUSB_DEBUG 0
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#define CFG_TUD_ENABLED 1
#define CFG_TUD_ENDPOINT0_SIZE 64

// Driver configuration
// Keyboard, generic, and raw HID interfaces + optionally log HID interface
#if defined(LOG_ENABLED)
#define CFG_TUD_HID 4
#else
#define CFG_TUD_HID 3
#endif

// HID buffer size. Must be at least the size of the largest reports (+1 for
// interface with multiple reports)
#define CFG_TUD_HID_EP_BUFSIZE 64

#if defined(BOARD_USB_FS)
#define BOARD_TUD_RHPORT 0
#elif defined(BOARD_USB_HS)
#define BOARD_TUD_RHPORT 1
#else
#error "USB peripheral not defined"
#endif
