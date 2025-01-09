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

#include <stdint.h>

//--------------------------------------------------------------------+
// Layout APIs
//--------------------------------------------------------------------+

/**
 * @brief Initialize the layout module
 *
 * @return none
 */
void layout_init(void);

/**
 * @brief Layout task
 *
 * This function is called in the main loop and handles the main logic of the
 * keyboard.
 *
 * @return none
 */
void layout_task(void);

/**
 * @brief Set the active keycode of a key
 *
 * When there is a release event of this key, this keycode will be removed from
 * the report.
 *
 * @param index The switch index
 * @param keycode The keycode
 *
 * @return none
 */
void layout_set_active_keycode(uint8_t index, uint16_t keycode);

/**
 * @brief Perform key press action of a keycode
 *
 * This function will not process advanced keycodes.
 *
 * @param keycode The keycode
 *
 * @return none
 */
void layout_key_press(uint16_t keycode);

/**
 * @brief Perform key release action of a keycode
 *
 * This function will not process advanced keycodes.
 *
 * @param keycode The keycode
 *
 * @return none
 */
void layout_key_release(uint16_t keycode);
