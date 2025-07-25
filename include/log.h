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
// Log API
//--------------------------------------------------------------------+

#if defined(LOG_ENABLED)
/**
 * @brief Initialize the log module
 *
 * @return None
 */
void log_init(void);

/**
 * @brief Log a format string to the log interface
 *
 * @param fmt Format string (printf-style)
 * @param ... Arguments for the format string
 *
 * @return Number of characters logged
 */
int log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Log task
 *
 * @return None
 */
void log_task(void);
#endif
