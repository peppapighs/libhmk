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

#include "common.h"

//--------------------------------------------------------------------+
// Timer API
//--------------------------------------------------------------------+

/**
 * @brief Initialize the timer module
 *
 * @return None
 */
void timer_init(void);

/**
 * @brief Read the current timer value
 *
 * @return Current timer value in milliseconds
 */
uint32_t timer_read(void);

/**
 * @brief Get the elapsed time since a given time
 *
 * @param since Time to compare against
 *
 * @return Elapsed time in milliseconds
 */
__attribute__((always_inline)) static inline uint32_t
timer_elapsed(uint32_t since) {
  return timer_read() - since;
}

/**
 * @brief Delay for a given amount of time
 *
 * @param ms Time to delay in milliseconds
 *
 * @return None
 */
__attribute__((always_inline)) static inline void timer_delay(uint32_t ms) {
  uint32_t start = timer_read();
  while (timer_elapsed(start) < ms)
    ;
}

/**
 * @brief Start the board buzzer sine wave
 *
 * @return None
 */
void timer_buzzer_start(void);

/**
 * @brief Stop the board buzzer sine wave
 *
 * @return None
 */
void timer_buzzer_stop(void);
