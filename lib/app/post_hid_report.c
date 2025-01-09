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

#include "post_hid_report.h"

#include <memory.h>

#include "layout.h"

static bool post_hid_report_unlocked;
static uint32_t post_hid_report_head;
static uint32_t num_post_hid_report_events;
static post_hid_report_event_t
    post_hid_report_events[MAX_POST_HID_REPORT_EVENTS];

#define POST_HID_REPORT_EVENT_FOREACH(i, event, block)                         \
    do {                                                                       \
        post_hid_report_unlocked = false;                                      \
        for (uint32_t(i) = 0; (i) < num_post_hid_report_events; (i)++) {       \
            const post_hid_report_event_t *(event) =                           \
                &post_hid_report_events[(post_hid_report_head + (i)) &         \
                                        (MAX_POST_HID_REPORT_EVENTS - 1)];     \
            (block);                                                           \
        }                                                                      \
        post_hid_report_unlocked = true;                                       \
    } while (0)

void post_hid_report_init(void) {
    post_hid_report_unlocked = true;
    post_hid_report_head = 0;
    num_post_hid_report_events = 0;
    memset(post_hid_report_events, 0, sizeof(post_hid_report_events));
}

/**
 * @brief Process a post-HID report event
 *
 * @param event The event
 *
 * @return none
 */
static void
post_hid_report_process_event(const post_hid_report_event_t *event) {
    switch (event->action) {
    case POST_HID_REPORT_ACTION_PRESS:
        layout_key_press(event->keycode);
        break;

    case POST_HID_REPORT_ACTION_RELEASE:
        layout_key_release(event->keycode);
        break;

    case POST_HID_REPORT_ACTION_TAP:
        if (post_hid_report_event_push(
                event->index, POST_HID_REPORT_ACTION_RELEASE, event->keycode))
            // Only perform the tap if the release event was successfully queued
            layout_key_press(event->keycode);
        break;

    default:
        break;
    }
}

bool post_hid_report_event_push(uint8_t index, uint8_t action,
                                uint16_t keycode) {
    if (!post_hid_report_unlocked)
        return false;

    post_hid_report_unlocked = false;

    if (num_post_hid_report_events >= MAX_POST_HID_REPORT_EVENTS) {
        // Process the oldest event
        post_hid_report_process_event(
            &post_hid_report_events[post_hid_report_head]);

        post_hid_report_head =
            (post_hid_report_head + 1) & (MAX_POST_HID_REPORT_EVENTS - 1);
        num_post_hid_report_events--;
    }

    const uint32_t tail = (post_hid_report_head + num_post_hid_report_events) &
                          (MAX_POST_HID_REPORT_EVENTS - 1);
    post_hid_report_events[tail].index = index;
    post_hid_report_events[tail].action = action;
    post_hid_report_events[tail].keycode = keycode;
    num_post_hid_report_events++;

    post_hid_report_unlocked = true;

    return true;
}

void post_hid_report_task(void) {
    static post_hid_report_event_t buffer[MAX_POST_HID_REPORT_EVENTS];

    if (!post_hid_report_unlocked || num_post_hid_report_events == 0)
        return;

    // Copy the events to a buffer so that the event queue can be unlocked
    const uint32_t len = num_post_hid_report_events;
    POST_HID_REPORT_EVENT_FOREACH(i, event, { buffer[i] = *event; });

    // Process the events
    post_hid_report_head = 0;
    num_post_hid_report_events = 0;

    for (uint32_t i = 0; i < len; i++)
        post_hid_report_process_event(&buffer[i]);
}
