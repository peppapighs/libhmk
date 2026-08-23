/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include <assert.h>
#include <stdio.h>

#define NUM_PROFILES 1
#define NUM_LAYERS 1
#define NUM_KEYS 1
#define NUM_ADVANCED_KEYS 1
#define NUM_DYNAMIC_KEYSTROKE_MAX_BINDINGS 4
#define NUM_MACRO_NODES 1
#define RGB_ENABLE
#define RGB_LED_COUNT 8

/*
 * Two rows of four LEDs wired as a serpentine: the first row runs left to
 * right, the second runs back right to left. Logical order stays row major.
 */
#define RGB_LED_INDEX_MAP {0, 1, 2, 3, 7, 6, 5, 4}
#define RGB_LED_POS_X {0, 85, 170, 255, 0, 85, 170, 255}

#include "../src/rgb.c"

const eeconfig_t *eeconfig;

static eeconfig_t test_config;
static uint8_t submitted_frame[RGB_FRAME_BYTES];
static uint32_t now_ms;

bool rgb_test_persist(const eeconfig_rgb_t *value) {
  assert(value != NULL);
  return true;
}

uint32_t timer_read(void) { return now_ms; }

bool rgb_backend_init(void) { return true; }

bool rgb_backend_submit(const uint8_t *rgb, uint16_t led_count) {
  assert(rgb != NULL);
  assert(led_count == RGB_LED_COUNT);
  memcpy(submitted_frame, rgb, sizeof(submitted_frame));
  return true;
}

bool rgb_backend_is_busy(void) { return false; }

void rgb_backend_task(void) {}

static const uint8_t *logical_pixel(uint8_t index) {
  return rgb_get_frame() + (uint16_t)index * RGB_BYTES_PER_PIXEL;
}

static const uint8_t *chain_pixel(uint8_t index) {
  return submitted_frame + (uint16_t)index * RGB_BYTES_PER_PIXEL;
}

static bool same_pixel(const uint8_t *a, const uint8_t *b) {
  return memcmp(a, b, RGB_BYTES_PER_PIXEL) == 0;
}

/* A logical write must reach the chain position the board declares. */
static void test_index_map_is_applied_at_the_output(void) {
  assert(rgb_set_effect(RGB_EFFECT_LIVE, false));
  assert(rgb_set_brightness(255u, false));
  assert(rgb_set_enabled(true, false));

  for (uint8_t i = 0; i < RGB_LED_COUNT; i++)
    assert(rgb_set_pixel(i, (uint8_t)(i + 1u), 0u, 0u));
  rgb_task();

  for (uint8_t i = 0; i < RGB_LED_COUNT; i++) {
    const uint8_t physical = rgb_led_index_map[i];
    assert(chain_pixel(physical)[0] == (uint8_t)(i + 1u));
  }
  /* Logical LED 4 opens the second row but is wired last. */
  assert(chain_pixel(7)[0] == 5u);
  assert(chain_pixel(4)[0] == 8u);
}

/*
 * The regression this file exists for: with the wave driven by the chain index
 * the second row runs backwards, so LEDs in the same column disagree.
 */
static void test_wave_follows_the_board_not_the_wiring(void) {
  assert(rgb_set_effect(RGB_EFFECT_RAINBOW_WAVE, false));

  for (uint8_t column = 0; column < 4u; column++) {
    assert(same_pixel(logical_pixel(column), logical_pixel(column + 4u)));
  }
  /* Neighbouring columns still differ, so the board is not a flat fill. */
  for (uint8_t column = 0; column + 1u < 4u; column++) {
    assert(!same_pixel(logical_pixel(column), logical_pixel(column + 1u)));
  }

  /* Advancing the phase shifts the same colors one step along the X axis. */
  uint8_t before[RGB_FRAME_BYTES];
  memcpy(before, rgb_get_frame(), sizeof(before));
  now_ms += 20u;
  rgb_task();
  assert(memcmp(before, rgb_get_frame(), sizeof(before)) != 0);
  for (uint8_t column = 0; column < 4u; column++) {
    assert(same_pixel(logical_pixel(column), logical_pixel(column + 4u)));
  }
}

/* Autonomous effects reach the strip through the same translation. */
static void test_static_fill_survives_the_translation(void) {
  assert(rgb_set_effect(RGB_EFFECT_STATIC, false));
  assert(rgb_fill(10u, 20u, 30u, false));
  rgb_task();

  for (uint8_t i = 0; i < RGB_LED_COUNT; i++) {
    assert(chain_pixel(i)[0] == 10u);
    assert(chain_pixel(i)[1] == 20u);
    assert(chain_pixel(i)[2] == 30u);
  }
}

int main(void) {
  test_config.rgb = (eeconfig_rgb_t){
      .enabled = 1u,
      .brightness = 255u,
      .effect = RGB_EFFECT_STATIC,
      .color_r = 255u,
      .color_g = 255u,
      .color_b = 255u,
  };
  eeconfig = &test_config;
  assert(rgb_init());

  test_index_map_is_applied_at_the_output();
  test_wave_follows_the_board_not_the_wiring();
  test_static_fill_survives_the_translation();

  printf("rgb topology tests passed\n");
  return 0;
}
