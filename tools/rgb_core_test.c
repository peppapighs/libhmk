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
#define RGB_LED_COUNT 21

#include "../src/rgb.c"

const eeconfig_t *eeconfig;

static eeconfig_t test_config;
static eeconfig_rgb_t last_persisted;
static uint8_t submitted_frame[RGB_FRAME_BYTES];
static uint32_t now_ms;
static uint32_t submit_count;
static bool persist_success = true;
static bool usb_suspended_state;

bool rgb_test_persist(const eeconfig_rgb_t *value) {
  assert(value != NULL);
  last_persisted = *value;
  return persist_success;
}

uint32_t timer_read(void) { return now_ms; }

bool tud_suspended(void) { return usb_suspended_state; }

bool rgb_backend_init(void) { return true; }

bool rgb_backend_submit(const uint8_t *rgb, uint16_t led_count) {
  assert(rgb != NULL);
  assert(led_count == RGB_LED_COUNT);
  memcpy(submitted_frame, rgb, sizeof(submitted_frame));
  submit_count++;
  return true;
}

bool rgb_backend_is_busy(void) { return false; }

void rgb_backend_task(void) {}

static void test_effects(void) {
  uint8_t wave_before[RGB_FRAME_BYTES];

  assert(rgb_set_effect(RGB_EFFECT_RAINBOW, false));
  assert(rgb_get_frame()[0] == 255u);
  assert(rgb_get_frame()[1] == 0u);
  assert(rgb_set_effect(RGB_EFFECT_RAINBOW_WAVE, false));
  assert(memcmp(rgb_get_frame(), rgb_get_frame() + RGB_BYTES_PER_PIXEL,
                RGB_BYTES_PER_PIXEL) != 0);
  assert(rgb_set_effect(RGB_EFFECT_BREATHING, false));
  assert(rgb_get_frame()[0] == 0u);
  now_ms += 20u;
  rgb_task();
  assert(rgb_get_frame()[0] != 0u);
  assert(!rgb_set_effect(4u, false));

  assert(rgb_set_effect(RGB_EFFECT_RAINBOW_WAVE, false));
  memcpy(wave_before, rgb_get_frame(), sizeof(wave_before));
  assert(rgb_fill(11u, 22u, 33u, true));
  assert(memcmp(wave_before, rgb_get_frame(), sizeof(wave_before)) == 0);
  assert(last_persisted.color_r == 11u);
  assert(last_persisted.color_g == 22u);
  assert(last_persisted.color_b == 33u);

  assert(rgb_set_effect(RGB_EFFECT_STATIC, false));
  assert(rgb_fill(7u, 8u, 9u, false));
  assert(rgb_get_frame()[0] == 7u);
  assert(rgb_get_frame()[1] == 8u);
  assert(rgb_get_frame()[2] == 9u);
}

static void test_hidden_animation_is_idle(void) {
  uint32_t idle_submit_count;

  assert(rgb_set_effect(RGB_EFFECT_BREATHING, false));
  rgb_task();
  assert(rgb_set_enabled(false, false));
  rgb_task();
  idle_submit_count = submit_count;
  now_ms += 100u;
  rgb_task();
  assert(submit_count == idle_submit_count);

  assert(rgb_set_enabled(true, false));
  assert(rgb_set_brightness(0u, false));
  rgb_task();
  idle_submit_count = submit_count;
  now_ms += 100u;
  rgb_task();
  assert(submit_count == idle_submit_count);
  assert(rgb_set_brightness(255u, false));

  assert(rgb_set_effect(RGB_EFFECT_RAINBOW, false));
  rgb_task();
  usb_suspended_state = true;
  rgb_task();
  for (uint32_t i = 0u; i < sizeof(submitted_frame); i++)
    assert(submitted_frame[i] == 0u);
  idle_submit_count = submit_count;
  now_ms += 100u;
  rgb_task();
  assert(submit_count == idle_submit_count);
  usb_suspended_state = false;
  rgb_task();
  assert(submit_count == idle_submit_count + 1u);
}

static void test_atomic_live_frame(void) {
  uint8_t frame[RGB_FRAME_BYTES];
  uint8_t late_chunk[RGB_FRAME_BYTES - RGB_FRAME_CHUNK_BYTES];
  uint8_t previous[RGB_FRAME_BYTES];

  for (uint32_t i = 0u; i < sizeof(frame); i++)
    frame[i] = (uint8_t)(i * 3u + 1u);
  memcpy(previous, rgb_get_frame(), sizeof(previous));

  assert(rgb_set_effect(RGB_EFFECT_RAINBOW_WAVE, false));
  assert(!rgb_set_frame_chunk(0u, frame, RGB_FRAME_CHUNK_BYTES));
  assert(!rgb_set_pixel(0u, 1u, 2u, 3u));
  assert(rgb_set_effect(RGB_EFFECT_LIVE, false));
  memcpy(previous, rgb_get_frame(), sizeof(previous));
  assert(rgb_set_frame_chunk(0u, frame, RGB_FRAME_CHUNK_BYTES));
  assert(memcmp(rgb_get_frame(), previous, sizeof(previous)) == 0);

  /* A direct live edit cancels an incomplete chunk transaction. */
  assert(rgb_set_pixel(0u, 9u, 8u, 7u));
  memcpy(previous, rgb_get_frame(), sizeof(previous));
  assert(rgb_set_frame_chunk(RGB_FRAME_CHUNK_BYTES,
                             frame + RGB_FRAME_CHUNK_BYTES,
                             RGB_FRAME_BYTES - RGB_FRAME_CHUNK_BYTES));
  assert(memcmp(rgb_get_frame(), previous, sizeof(previous)) == 0);

  assert(rgb_set_frame_chunk(0u, frame, RGB_FRAME_CHUNK_BYTES));
  assert(rgb_set_frame_chunk(RGB_FRAME_CHUNK_BYTES,
                             frame + RGB_FRAME_CHUNK_BYTES,
                             RGB_FRAME_BYTES - RGB_FRAME_CHUNK_BYTES));
  assert(memcmp(rgb_get_frame(), frame, sizeof(frame)) == 0);
  rgb_task();
  const uint32_t completed_publish_count = submit_count;

  /* Publication closes the chunk transaction. A delayed non-zero chunk from
   * another frame cannot modify or republish the displayed frame. */
  memset(late_chunk, 0xa5, sizeof(late_chunk));
  assert(rgb_set_frame_chunk(RGB_FRAME_CHUNK_BYTES, late_chunk,
                             sizeof(late_chunk)));
  assert(memcmp(rgb_get_frame(), frame, sizeof(frame)) == 0);
  rgb_task();
  assert(submit_count == completed_publish_count);

  /* A direct fill likewise cancels a partial upload. */
  assert(rgb_set_frame_chunk(0u, frame, RGB_FRAME_CHUNK_BYTES));
  assert(rgb_fill(4u, 5u, 6u, false));
  memcpy(previous, rgb_get_frame(), sizeof(previous));
  assert(rgb_set_frame_chunk(RGB_FRAME_CHUNK_BYTES,
                             frame + RGB_FRAME_CHUNK_BYTES,
                             RGB_FRAME_BYTES - RGB_FRAME_CHUNK_BYTES));
  assert(memcmp(rgb_get_frame(), previous, sizeof(previous)) == 0);

  /* Leaving and re-entering live mode also discards an incomplete upload. */
  assert(rgb_restore_effect());
  assert(rgb_set_effect(RGB_EFFECT_LIVE, false));
  memcpy(previous, rgb_get_frame(), sizeof(previous));
  assert(rgb_set_frame_chunk(RGB_FRAME_CHUNK_BYTES, late_chunk,
                             sizeof(late_chunk)));
  assert(memcmp(rgb_get_frame(), previous, sizeof(previous)) == 0);
  assert(!rgb_set_frame_chunk(1u, frame, RGB_FRAME_CHUNK_BYTES));

  assert(rgb_set_brightness(128u, true));
  assert(last_persisted.effect == RGB_EFFECT_RAINBOW_WAVE);
  assert(rgb_get_effect() == RGB_EFFECT_LIVE);
  assert(rgb_restore_effect());
  assert(rgb_get_effect() == RGB_EFFECT_RAINBOW_WAVE);
}

static void test_persistence_failure_is_transactional(void) {
  uint8_t before[RGB_FRAME_BYTES];

  assert(rgb_set_effect(RGB_EFFECT_STATIC, false));
  assert(rgb_fill(20u, 30u, 40u, false));
  assert(rgb_set_enabled(true, false));
  assert(rgb_set_brightness(200u, false));
  memcpy(before, rgb_get_frame(), sizeof(before));

  persist_success = false;
  assert(!rgb_set_enabled(false, true));
  assert(rgb_is_enabled());
  assert(!rgb_set_brightness(10u, true));
  assert(rgb_get_brightness() == 200u);
  assert(!rgb_set_effect(RGB_EFFECT_BREATHING, true));
  assert(rgb_get_effect() == RGB_EFFECT_STATIC);
  assert(!rgb_fill(1u, 2u, 3u, true));
  assert(memcmp(before, rgb_get_frame(), sizeof(before)) == 0);
  persist_success = true;
}

int main(void) {
  test_config.rgb = (eeconfig_rgb_t){
      .enabled = 1u,
      .brightness = 255u,
      .effect = RGB_EFFECT_STATIC,
      .color_r = 120u,
      .color_g = 40u,
      .color_b = 10u,
  };
  eeconfig = &test_config;

  assert(rgb_init());
  rgb_task();
  assert(submit_count == 1u);
  assert(submitted_frame[0] == 120u);
  assert(submitted_frame[1] == 40u);
  assert(submitted_frame[2] == 10u);

  test_effects();
  test_hidden_animation_is_idle();
  test_atomic_live_frame();
  test_persistence_failure_is_transactional();
  puts("rgb_core_test: ok");
  return 0;
}
