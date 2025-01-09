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

#include "user_config.h"

//--------------------------------------------------------------------+
// Tap-Hold Types
//--------------------------------------------------------------------+

typedef struct {
    // The switch index
    uint8_t index;
    // The tap-hold configuration. `NULL` if the data is empty.
    const advanced_key_tap_hold_t *ak_th;
    // The time when the tap-hold event was registered
    uint32_t since;
} tap_hold_data_t;

//--------------------------------------------------------------------+
// Tap-Hold APIs
//--------------------------------------------------------------------+

/**
 * @brief Initialize the tap-hold module
 *
 * @return none
 */
void tap_hold_init(void);

/**
 * @brief Register a tap-hold key
 *
 * @param ak_index The advanced key configuration index
 * @param index The switch index
 * @param ak_th The tap-hold configuration
 * @param since The time when the tap-hold key was registered
 *
 * @return none
 */
void tap_hold_register(uint8_t ak_index, uint8_t index,
                       const advanced_key_tap_hold_t *ak_th, uint32_t since);

/**
 * @brief Tap-hold task
 *
 * This function is called after the matrix scan periodically and handles the
 * tap-hold logic. It will lock the tap-hold data while processing the events.
 *
 * @param has_press_event Whether there is a press event
 * @param has_release_event The bitmap of the release events
 *
 * @return none
 */
void tap_hold_task(bool has_press_event, const uint32_t *has_release_event);
