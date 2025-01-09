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
// Layer APIs
//--------------------------------------------------------------------+

/**
 * @brief Initialize the layout layer
 *
 * @return none
 */
void layer_init(void);

/**
 * @brief Get the current layer
 *
 * @return The current layer
 */
uint8_t get_current_layer(void);

/**
 * @brief Activate a layer
 *
 * @param layer_num The layer to activate
 *
 * @return none
 */
void layer_on(uint8_t layer_num);

/**
 * @brief Deactivate a layer
 *
 * @param layer_num The layer to deactivate
 *
 * @return none
 */
void layer_off(uint8_t layer_num);

/**
 * @brief Toggle a layer
 *
 * @param layer_num The layer to toggle
 *
 * @return none
 */
void layer_toggle(uint8_t layer_num);

/**
 * @brief Go to a layer
 *
 * @param layer_num The layer to go to
 *
 * @return none
 */
void layer_goto(uint8_t layer_num);

/**
 * @brief Set the default layer
 *
 * @param layer_num The default layer
 *
 * @return none
 */
void set_default_layer(uint8_t layer_num);

/**
 * @brief Get the keycode at the specified switch index
 *
 * @param index The switch index
 *
 * @return The keycode
 */
uint16_t get_keycode(uint8_t index);
