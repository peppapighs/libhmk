/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "hardware/hardware.h"

#include "stm32f7xx_hal.h"

void timer_init(void) {}

uint32_t timer_read(void) { return HAL_GetTick(); }
