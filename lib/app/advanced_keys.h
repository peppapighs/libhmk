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
// Advanced Key Types
//--------------------------------------------------------------------+

typedef struct {
    // The advanced key configuration. `NULL` if not registered.
    const advanced_key_config_t *config;
    // The switch index
    uint8_t index;
    // The time when the tap-hold event was registered
    uint32_t since;
} advanced_key_data_t;

//--------------------------------------------------------------------+
// Advanced Key APIs
//--------------------------------------------------------------------+

/**
 * @brief Initialize the advanced key module
 *
 * @return none
 */
void advanced_key_init(void);

/**
 * @brief Advanced key task
 *
 * @param ak_index The advanced key configuration index
 * @param index The switch index
 * @param sw_state The current switch state
 * @param last_sw_state The last switch state
 * @param since The time when the event was registered
 *
 * @return none
 */
void advanced_key_task(uint8_t ak_index, uint8_t index, uint8_t sw_state,
                       uint8_t last_sw_state, uint32_t since);

/**
 * @brief Tick task for the advanced key module
 *
 * This function is called after the matrix scan periodically and handles the
 * tap-hold and toggle keys.
 *
 * @param has_press_event Whether there is a press event
 * @param has_release_event The bitmap of the release events
 *
 * @return none
 */
void advanced_key_tick_task(bool has_press_event,
                            const uint32_t *has_release_event);
