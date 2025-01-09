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

#include "layout.h"

#include <memory.h>
#include <stdbool.h>

#include "advanced_keys.h"
#include "board.h"
#include "config.h"
#include "hid.h"
#include "keycodes.h"
#include "layer.h"
#include "switches.h"
#include "timer.h"
#include "user_config.h"

#if defined(DEBUG)
#include "debug.h"
#endif

#if defined(DEBUG)
static uint32_t debug_input_latency;
#endif

static bool should_send_reports;
static uint16_t active_keycodes[NUM_KEYS];
static uint8_t last_sw_states[NUM_KEYS];

void layout_init(void) {
    should_send_reports = false;
    memset(active_keycodes, 0, sizeof(active_keycodes));
    memset(last_sw_states, 0, sizeof(last_sw_states));
}

void layout_task(void) {
    static uint32_t last_tick_event = 0;
    static uint32_t has_release_event[(NUM_KEYS + 31) / 32];

#if defined(DEBUG)
    uint32_t debug_start = debug_get_counter();
#endif

    matrix_scan();

    bool matrix_changed = false;
    bool has_press_event = false;

    // Clear the bitmap
    for (uint32_t i = 0; i < (NUM_KEYS + 31) / 32; i++)
        has_release_event[i] = 0;

    // Check if there are any pressed/released events
    for (uint32_t i = 0; i < NUM_KEYS; i++) {
        const uint8_t sw_state = get_switch_state(i);

        if (sw_state != last_sw_states[i]) {
            matrix_changed = true;
            has_press_event |= sw_state;
            has_release_event[i >> 5] |= (uint32_t)(!sw_state) << (i & 0x1F);
        }
    }

    if (matrix_changed || timer_elapsed(last_tick_event) > 0) {
        advanced_key_tick_task(has_press_event, has_release_event);
        last_tick_event = timer_read();
    }

    for (uint32_t i = 0; i < NUM_KEYS; i++) {
        const uint8_t sw_state = get_switch_state(i);

        if (sw_state && !last_sw_states[i]) {
            // Key press event
            const uint16_t keycode = get_keycode(i);
            active_keycodes[i] = keycode;

            if (IS_ADVANCED_KEY_KEYCODE(keycode)) {
                advanced_key_task(SP_ADVANCED_KEY_GET_INDEX(keycode), i,
                                  sw_state, last_sw_states[i], timer_read());
            } else {
                layout_key_press(keycode);
            }
        } else if (!sw_state && last_sw_states[i]) {
            // Key release event
            const uint16_t keycode = active_keycodes[i];
            active_keycodes[i] = KC_NO;

            if (IS_ADVANCED_KEY_KEYCODE(keycode)) {
                advanced_key_task(SP_ADVANCED_KEY_GET_INDEX(keycode), i,
                                  sw_state, last_sw_states[i], timer_read());
            } else {
                layout_key_release(keycode);
            }
        } else {
            const uint16_t keycode = active_keycodes[i];

            if (IS_ADVANCED_KEY_KEYCODE(keycode))
                advanced_key_task(SP_ADVANCED_KEY_GET_INDEX(keycode), i,
                                  sw_state, last_sw_states[i], timer_read());
        }

        // Update the last switch state
        last_sw_states[i] = sw_state;
    }

    if (should_send_reports) {
        hid_send_reports();
        should_send_reports = false;
    }

#if defined(DEBUG)
    debug_input_latency = DEBUG_COUNTER_DIFF(debug_get_counter(), debug_start);
#endif
}

void layout_should_send_reports(void) { should_send_reports = true; }

void layout_set_active_keycode(uint8_t index, uint16_t keycode) {
    active_keycodes[index] = keycode;
}

void layout_key_press(uint16_t keycode) {
    if (IS_HID_KEYCODE(keycode)) {
        hid_add_keycode(keycode);
        // We should send the reports after the key is added to the report.
        should_send_reports = true;
    } else if (IS_LAYER_TO_KEYCODE(keycode)) {
        layer_goto(SP_LAYER_TO_GET_LAYER(keycode));
    } else if (IS_LAYER_MO_KEYCODE(keycode)) {
        layer_on(SP_LAYER_MO_GET_LAYER(keycode));
    } else if (IS_LAYER_DEF_KEYCODE(keycode)) {
        set_default_layer(SP_LAYER_DEF_GET_LAYER(keycode));
    } else if (IS_LAYER_TOGGLE_KEYCODE(keycode)) {
        layer_toggle(SP_LAYER_TOGGLE_GET_LAYER(keycode));
    } else if (IS_PROFILE_TO_KEYCODE(keycode)) {
        user_config_set_current_profile(SP_PROFILE_TO_GET_PROFILE(keycode));
    } else if (IS_MAGIC_KEYCODE(keycode)) {
        switch (keycode) {
#if defined(ENABLE_BOOTLOADER)
        case SP_MAGIC_BOOTLOADER:
            board_enter_bootloader();
            break;
#endif

        case SP_MAGIC_REBOOT:
            board_reset();
            break;

        case SP_MAGIC_FACTORY_RESET:
            user_config_reset();
            break;

        case SP_MAGIC_RECALIBRATE:
            switch_recalibrate();
            break;

        default:
            break;
        }
    }
}

void layout_key_release(uint16_t keycode) {
    if (IS_HID_KEYCODE(keycode)) {
        hid_remove_keycode(keycode);
        // We should send the reports after the key is removed from the report.
        should_send_reports = true;
    } else if (IS_LAYER_TO_KEYCODE(keycode)) {
        // Nothing to do
    } else if (IS_LAYER_MO_KEYCODE(keycode)) {
        layer_off(SP_LAYER_MO_GET_LAYER(keycode));
    } else if (IS_LAYER_DEF_KEYCODE(keycode)) {
        // Nothing to do
    } else if (IS_LAYER_TOGGLE_KEYCODE(keycode)) {
        // Nothing to do
    } else if (IS_PROFILE_TO_KEYCODE(keycode)) {
        // Nothing to do
    } else if (IS_MAGIC_KEYCODE(keycode)) {
        // Nothing to do
    }
}
