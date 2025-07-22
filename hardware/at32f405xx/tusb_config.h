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

// STM32F446xx
#define CFG_TUSB_MCU OPT_MCU_AT32F402_405

// Default TinyUSB configuration
#define CFG_TUSB_OS OPT_OS_NONE
#define CFG_TUSB_DEBUG 0
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#define CFG_TUD_ENABLED 1
#define CFG_TUD_ENDPOINT0_SIZE 64

// Driver configuration
#if defined(LOG_ENABLED)
// 4 HID interfaces (keyboard, generic, raw HID, and log)
#define CFG_TUD_HID 4
#else
// 3 HID interfaces (keyboard, generic, and raw HID)
#define CFG_TUD_HID 3
#endif

// HID buffer size. Must be at least the size of the largest reports (+1 for
// interface with multiple reports)
#define CFG_TUD_HID_EP_BUFSIZE 64

#if defined(BOARD_USB_FS)
#define BOARD_TUD_RHPORT 0
#elif defined(BOARD_USB_HS)
#define BOARD_TUD_RHPORT 1

_Static_assert(
    BOARD_HSE_VALUE == 12000000,
    "High-speed external crystal frequency must be 12 MHz for USB HS");
#else
#error "USB peripheral not defined"
#endif
