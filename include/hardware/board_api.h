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
// Board-Specific API
//--------------------------------------------------------------------+

/**
 * @brief Initialize the board
 *
 * This function should initialize the clock, GPIO, and any other peripherals
 * needed by the board. It will be called at the beginning of the program.
 *
 * @return None
 */
void board_init(void);

/**
 * @brief Error handler
 *
 * This function should be called when an unrecoverable error occurs. It should
 * not return.
 *
 * @return None
 */
void board_error_handler(void);

/**
 * @brief Reset the board
 *
 * This function should not return.
 *
 * @return None
 */
void board_reset(void);

/**
 * @brief Enter the bootloader mode
 *
 * If bootloader mode is supported, this function should not return.
 *
 * @return None
 */
void board_enter_bootloader(void);

/**
 * @brief Get the board serial number
 *
 * The serial number is a UTF-8 string with a maximum length of 32 and no null
 * terminator.
 *
 * @param buf Buffer to store the serial number
 *
 * @return Length of the serial number in characters
 */
uint32_t board_serial(char *buf);

/**
 * @brief Get the current cycle count
 *
 * This function returns the current cycle count of the board's CPU. It can be
 * used for performance measurement or timing purposes.
 *
 * @return Current cycle count
 */
uint32_t board_cycle_count(void);
