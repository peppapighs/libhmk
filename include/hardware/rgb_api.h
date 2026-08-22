/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#pragma once

#include "common.h"

#if defined(RGB_ENABLE)

/* Backends must return immediately. `rgb` remains valid only for the duration
 * of this call, so a backend must copy it before returning true. */
bool rgb_backend_init(void);
bool rgb_backend_submit(const uint8_t *rgb, uint16_t led_count);
bool rgb_backend_is_busy(void);
void rgb_backend_task(void);

#endif
