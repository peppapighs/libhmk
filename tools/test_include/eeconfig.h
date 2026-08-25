/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#pragma once

#include "common.h"

typedef struct {
  uint8_t enabled;
  uint8_t brightness;
  uint8_t effect;
  uint8_t color_r;
  uint8_t color_g;
  uint8_t color_b;
} eeconfig_rgb_t;

typedef struct {
  eeconfig_rgb_t rgb;
} eeconfig_t;

extern const eeconfig_t *eeconfig;
bool rgb_test_persist(const eeconfig_rgb_t *value);

#define EECONFIG_WRITE(field, value) rgb_test_persist(value)
