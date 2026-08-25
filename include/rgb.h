/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#pragma once

#include "common.h"

#if defined(RGB_ENABLE)

#if !defined(RGB_LED_COUNT)
#error "RGB_LED_COUNT is not defined"
#endif

#define RGB_BYTES_PER_PIXEL 3u
#define RGB_FRAME_BYTES ((uint16_t)(RGB_LED_COUNT * RGB_BYTES_PER_PIXEL))
#define RGB_FRAME_CHUNK_BYTES 60u
#define RGB_EFFECT_STATIC 0u
#define RGB_EFFECT_BREATHING 1u
#define RGB_EFFECT_RAINBOW 2u
#define RGB_EFFECT_RAINBOW_WAVE 3u
#define RGB_EFFECT_LIVE 7u

bool rgb_init(void);
void rgb_task(void);

bool rgb_is_enabled(void);
bool rgb_set_enabled(bool enabled, bool persist);
uint8_t rgb_get_brightness(void);
bool rgb_set_brightness(uint8_t brightness, bool persist);
uint8_t rgb_get_effect(void);
bool rgb_set_effect(uint8_t effect, bool persist);
bool rgb_restore_effect(void);

const uint8_t *rgb_get_frame(void);
bool rgb_get_pixel(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b);
bool rgb_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
bool rgb_get_frame_chunk(uint16_t offset, uint8_t *dst, uint8_t len);
bool rgb_set_frame_chunk(uint16_t offset, const uint8_t *src, uint8_t len);
bool rgb_fill(uint8_t r, uint8_t g, uint8_t b, bool persist);

#endif
