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

#include "layer.h"

#include "keycodes.h"
#include "user_config.h"

static uint8_t default_layer_num;
static uint16_t layer_mask;

void layer_init(void) {
    default_layer_num = 0;
    layer_mask = 0;
}

uint8_t get_current_layer(void) {
    if (!layer_mask)
        return default_layer_num;

    return 31 - __builtin_clz(layer_mask);
}

void layer_on(uint8_t layer_num) { layer_mask |= 1 << layer_num; }

void layer_off(uint8_t layer_num) { layer_mask &= ~(1 << layer_num); }

void layer_toggle(uint8_t layer_num) { layer_mask ^= 1 << layer_num; }

void layer_goto(uint8_t layer_num) { layer_mask = 1 << layer_num; }

void set_default_layer(uint8_t layer_num) { default_layer_num = layer_num; }

uint16_t get_keycode(uint8_t index) {
    const uint8_t current_profile = user_config_current_profile();

    // Find the highest active layer with a non-transparent keycode
    uint8_t mask = 1 << get_current_layer();
    for (uint8_t i = get_current_layer() + 1; i-- > 0; mask >>= 1) {
        if (!(layer_mask & mask))
            continue;

        const uint16_t keycode = user_config_keymap(current_profile, i, index);
        if (keycode != KC_TRANSPARENT)
            return keycode;
    }

    // Otherwise, use the default layer
    return user_config_keymap(current_profile, default_layer_num, index);
}