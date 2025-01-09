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

#include "advanced_keys.h"

#include <memory.h>

#include "keycodes.h"
#include "layout.h"
#include "post_hid_report.h"
#include "timer.h"

static bool advanced_key_unlocked;
static advanced_key_data_t advanced_key_data[NUM_ADVANCED_KEY_BINDINGS];

#define ADVANCED_KEY_DATA_FOREACH(i, data, block)                              \
    do {                                                                       \
        advanced_key_unlocked = false;                                         \
        for (uint32_t(i) = 0; (i) < NUM_ADVANCED_KEY_BINDINGS; (i)++) {        \
            const advanced_key_data_t *(data) = &advanced_key_data[(i)];       \
            if ((data)->config == NULL)                                        \
                continue;                                                      \
            (block);                                                           \
        }                                                                      \
        advanced_key_unlocked = true;                                          \
    } while (0)

void advanced_key_init(void) {
    advanced_key_unlocked = true;
    memset(advanced_key_data, 0, sizeof(advanced_key_data));
}

void advanced_key_task(uint8_t ak_index, uint8_t index, uint8_t sw_state,
                       uint8_t last_sw_state, uint32_t since) {
    if (ak_index >= NUM_ADVANCED_KEY_BINDINGS || !advanced_key_unlocked)
        return;

    advanced_key_unlocked = false;

    const uint8_t current_profile = user_config_current_profile();
    const advanced_key_config_t *config =
        user_config_get_advanced_key_config(current_profile, ak_index);

    switch (config->type) {
    case ADVANCED_KEY_TAP_HOLD:
        if (sw_state && !last_sw_state) {
            // Tap-hold key is pressed
            advanced_key_data[ak_index].config = config;
            advanced_key_data[ak_index].index = index;
            advanced_key_data[ak_index].since = since;
        }
        break;

    default:
        break;
    }

    advanced_key_unlocked = true;
}

/**
 * @brief Execute the tap action of a tap-hold event
 *
 * @param data The advanced key data. Must be a tap-hold key.
 *
 * @return none
 */
static void tap_hold_tap_action(const advanced_key_data_t *data) {
    if (post_hid_report_event_push(data->index, POST_HID_REPORT_ACTION_RELEASE,
                                   data->config->th.tap_keycode))
        // No need to set the active keycode since post-HID-report task will
        // handle the release event
        layout_key_press(data->config->th.tap_keycode);
}

/**
 * @brief Execute the hold action of a tap-hold event
 *
 * @param data The advanced key data. Must be a tap-hold key.
 *
 * @return none
 */
static void tap_hold_hold_action(const advanced_key_data_t *data) {
    layout_set_active_keycode(data->index, data->config->th.hold_keycode);
    layout_key_press(data->config->th.hold_keycode);
}

void advanced_key_tick_task(bool has_press_event,
                            const uint32_t *has_release_event) {
    if (!advanced_key_unlocked)
        return;

    ADVANCED_KEY_DATA_FOREACH(i, data, {
        bool should_pop = false;

        switch (data->config->type) {
        case ADVANCED_KEY_TAP_HOLD:
            const advanced_key_tap_hold_t *th = &data->config->th;
            const uint16_t tapping_term = th->tapping_term;

            if (has_press_event && IS_MODIFIER_KEYCODE(th->hold_keycode)) {
                // The tap-hold key has a modifier as the hold keycode and
                // another key is pressed so immediately perform the hold
                // action.
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

            break;

        default:
            break;
        }

        if (should_pop)
            advanced_key_data[i].config = NULL;
    });
}
