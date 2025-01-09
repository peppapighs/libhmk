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

#include "tap_hold.h"

#include <memory.h>

#include "keycodes.h"
#include "layout.h"
#include "post_hid_report.h"
#include "timer.h"

static bool tap_hold_unlocked;
static tap_hold_data_t tap_hold_data[NUM_ADVANCED_KEY_BINDINGS];

#define TAP_HOLD_DATA_FOREACH(i, data, block)                                  \
    do {                                                                       \
        tap_hold_unlocked = false;                                             \
        for (uint32_t(i) = 0; (i) < NUM_ADVANCED_KEY_BINDINGS; (i)++) {        \
            const tap_hold_data_t *(data) = &tap_hold_data[(i)];               \
            if ((data)->ak_th == NULL)                                         \
                continue;                                                      \
            (block);                                                           \
        }                                                                      \
        tap_hold_unlocked = true;                                              \
    } while (0)

void tap_hold_init(void) {
    tap_hold_unlocked = true;
    for (uint32_t i = 0; i < NUM_ADVANCED_KEY_BINDINGS; i++)
        tap_hold_data[i].ak_th = NULL;
}

void tap_hold_register(uint8_t ak_index, uint8_t index,
                       const advanced_key_tap_hold_t *ak_th, uint32_t since) {
    if (!tap_hold_unlocked)
        return;

    tap_hold_unlocked = false;

    tap_hold_data[ak_index].index = index;
    tap_hold_data[ak_index].ak_th = ak_th;
    tap_hold_data[ak_index].since = since;

    tap_hold_unlocked = true;
}

/**
 * @brief Execute the tap action of a tap-hold event
 *
 * @param data The tap-hold data
 *
 * @return none
 */
static void tap_hold_tap_action(const tap_hold_data_t *data) {
    if (post_hid_report_event_push(data->index, POST_HID_REPORT_ACTION_RELEASE,
                                   data->ak_th->tap_keycode))
        // No need to set the active keycode since post-HID-report task will
        // handle the release event
        layout_key_press(data->ak_th->tap_keycode);
}

/**
 * @brief Execute the hold action of a tap-hold event
 *
 * @param data The tap-hold data
 *
 * @return none
 */
static void tap_hold_hold_action(const tap_hold_data_t *data) {
    layout_set_active_keycode(data->index, data->ak_th->hold_keycode);
    layout_key_press(data->ak_th->hold_keycode);
}

void tap_hold_task(bool has_press_event, const uint32_t *has_release_event) {
    if (!tap_hold_unlocked)
        return;

    TAP_HOLD_DATA_FOREACH(i, data, {
        const uint16_t tapping_term = data->ak_th->tapping_term;

        bool should_pop = false;
        if (has_press_event && IS_MODIFIER_KEYCODE(data->ak_th->hold_keycode)) {
            // The tap-hold key has a modifier as the hold keycode and another
            // key is pressed so immediately perform the hold action.
            tap_hold_hold_action(data);
            should_pop = true;
        } else {
            const uint32_t elapsed = timer_elapsed(data->since);
            const bool release = has_release_event[data->index >> 5] &
                                 (1 << (data->index & 0x1F));

            if (release && elapsed < tapping_term) {
                // Release the key before the tapping term expires
                tap_hold_tap_action(data);
                should_pop = true;
            } else if (!release && elapsed >= tapping_term) {
                // Hold the key after the tapping term expires
                tap_hold_hold_action(data);
                should_pop = true;
            }
        }

        if (should_pop)
            tap_hold_data[i].ak_th = NULL;
    });
}
