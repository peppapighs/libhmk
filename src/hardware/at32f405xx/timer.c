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

#include "hardware/hardware.h"

#include "at32f402_405.h"

static volatile uint32_t counter;

void timer_init(void) { SysTick_Config(system_core_clock / 1000); }

uint32_t timer_read(void) { return counter; }

//--------------------------------------------------------------------+
// Interrupt Handlers
//--------------------------------------------------------------------+

void SysTick_Handler(void) { counter++; }
