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

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

//--------------------------------------------------------------------+
// Post-HID-Report Configuration
//--------------------------------------------------------------------+

#if !defined(MAX_POST_HID_REPORT_EVENTS)
// The maximum number of post-HID report events in the event queue per matrix
// scan
#define MAX_POST_HID_REPORT_EVENTS 32
#endif

_Static_assert((MAX_POST_HID_REPORT_EVENTS &
                (MAX_POST_HID_REPORT_EVENTS - 1)) == 0,
               "MAX_POST_HID_REPORT_EVENTS must be a power of 2");

//--------------------------------------------------------------------+
// Post-HID-Report Types
//--------------------------------------------------------------------+

enum {
    POST_HID_REPORT_ACTION_PRESS = 0,
    POST_HID_REPORT_ACTION_RELEASE,
    POST_HID_REPORT_ACTION_TAP,
};

typedef struct {
    // The switch index
    uint8_t index;
    // The action to perform
    uint8_t action;
    // The keycode of the event
    uint16_t keycode;
} post_hid_report_event_t;

//--------------------------------------------------------------------+
// Post-HID-Report APIs
//--------------------------------------------------------------------+

/**
 * @brief Initialize the post-HID-report module
 *
 * @return none
 */
void post_hid_report_init(void);

/**
 * @brief Push a post-HID-report event to the event queue
 *
 * If the queue is locked, the event will be ignored. If the queue is full, the
 * oldest event will be processed.
 *
 * @param index The switch index
 * @param action The action to perform
 * @param keycode The keycode of the event
 *
 * @return Whether the event was pushed
 */
bool post_hid_report_event_push(uint8_t index, uint8_t action,
                                uint16_t keycode);

/**
 * @brief Post-HID-report task
 *
 * This function is called after all the HID reports are sent and clears the
 * event queue.
 *
 * @return none
 */
void post_hid_report_task(void);