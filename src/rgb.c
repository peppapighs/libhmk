/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "rgb.h"

#if defined(RGB_ENABLE)

#include "eeconfig.h"
#include "hardware/rgb_api.h"
#include "hardware/timer_api.h"

_Static_assert(RGB_LED_COUNT <= 255, "RGB bridge encodes LED count in one byte");
_Static_assert((RGB_FRAME_BYTES + RGB_FRAME_CHUNK_BYTES - 1u) /
                       RGB_FRAME_CHUNK_BYTES <=
                   16u,
               "RGB live-frame chunk bitmap is too small");

/*
 * Optional board topology. `RGB_LED_INDEX_MAP` maps a logical LED to its place
 * in the physical chain, and `RGB_LED_POS_X` gives each logical LED a 0-255
 * coordinate along the board's width. A board that declares neither keeps the
 * previous behaviour: the strip is a bare line of pixels in wiring order.
 */
#if defined(RGB_LED_INDEX_MAP)
static const uint8_t rgb_led_index_map[RGB_LED_COUNT] = RGB_LED_INDEX_MAP;
#endif
#if defined(RGB_LED_POS_X)
static const uint8_t rgb_led_pos_x[RGB_LED_COUNT] = RGB_LED_POS_X;
#endif

static inline uint16_t rgb_physical_index(uint16_t index) {
#if defined(RGB_LED_INDEX_MAP)
  return rgb_led_index_map[index];
#else
  return index;
#endif
}

/*
 * Phase offset of one LED in a travelling effect. Spreading the offset over the
 * board's X axis makes the wave a vertical band sweeping horizontally. Without
 * declared positions the offset falls back to the chain index, which on a
 * serpentine strip looks like a snake rather than a wave.
 */
static inline uint8_t rgb_wave_offset(uint16_t index) {
#if defined(RGB_LED_POS_X)
  return rgb_led_pos_x[index];
#else
  return (uint8_t)(((uint32_t)index * 256u) / RGB_LED_COUNT);
#endif
}

static eeconfig_rgb_t rgb_config;
static uint8_t rgb_frame[RGB_FRAME_BYTES];
static uint8_t rgb_live_staging[RGB_FRAME_BYTES];
static uint8_t rgb_output[RGB_FRAME_BYTES];
static uint8_t previous_effect;
static uint8_t effect_phase;
static uint16_t live_chunks_received;
static uint32_t last_effect_ms;
static bool frame_dirty;

static bool rgb_effect_is_valid(uint8_t effect) {
  return effect == RGB_EFFECT_STATIC || effect == RGB_EFFECT_BREATHING ||
         effect == RGB_EFFECT_RAINBOW ||
         effect == RGB_EFFECT_RAINBOW_WAVE || effect == RGB_EFFECT_LIVE;
}

static void rgb_write_pixel(uint8_t *frame, uint16_t index, uint8_t r,
                            uint8_t g, uint8_t b) {
  frame[index * RGB_BYTES_PER_PIXEL] = r;
  frame[index * RGB_BYTES_PER_PIXEL + 1u] = g;
  frame[index * RGB_BYTES_PER_PIXEL + 2u] = b;
}

static void rgb_fill_frame(uint8_t *frame, uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < RGB_LED_COUNT; i++)
    rgb_write_pixel(frame, i, r, g, b);
}

/* Compact integer HSV conversion for portable effects. Saturation is fixed at
 * 100%; callers control value through the global brightness stage. */
static void rgb_hue_to_rgb(uint8_t hue, uint8_t *r, uint8_t *g, uint8_t *b) {
  const uint8_t region = hue / 43u;
  const uint8_t offset = (uint8_t)((hue - region * 43u) * 6u);
  const uint8_t rising = offset;
  const uint8_t falling = (uint8_t)(255u - offset);

  switch (region) {
  case 0:
    *r = 255u;
    *g = rising;
    *b = 0u;
    break;
  case 1:
    *r = falling;
    *g = 255u;
    *b = 0u;
    break;
  case 2:
    *r = 0u;
    *g = 255u;
    *b = rising;
    break;
  case 3:
    *r = 0u;
    *g = falling;
    *b = 255u;
    break;
  case 4:
    *r = rising;
    *g = 0u;
    *b = 255u;
    break;
  default:
    *r = 255u;
    *g = 0u;
    *b = falling;
    break;
  }
}

static void rgb_render_effect(void) {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  switch (rgb_config.effect) {
  case RGB_EFFECT_STATIC:
    rgb_fill_frame(rgb_frame, rgb_config.color_r, rgb_config.color_g,
                   rgb_config.color_b);
    break;
  case RGB_EFFECT_BREATHING: {
    const uint8_t value = effect_phase < 128u
                              ? (uint8_t)(effect_phase * 2u)
                              : (uint8_t)((255u - effect_phase) * 2u);
    r = (uint8_t)(((uint32_t)rgb_config.color_r * (uint32_t)value + 127u) /
                  255u);
    g = (uint8_t)(((uint32_t)rgb_config.color_g * (uint32_t)value + 127u) /
                  255u);
    b = (uint8_t)(((uint32_t)rgb_config.color_b * (uint32_t)value + 127u) /
                  255u);
    rgb_fill_frame(rgb_frame, r, g, b);
    break;
  }
  case RGB_EFFECT_RAINBOW:
    rgb_hue_to_rgb(effect_phase, &r, &g, &b);
    rgb_fill_frame(rgb_frame, r, g, b);
    break;
  case RGB_EFFECT_RAINBOW_WAVE:
    for (uint16_t i = 0; i < RGB_LED_COUNT; i++) {
      const uint8_t hue = (uint8_t)(effect_phase + rgb_wave_offset(i));
      rgb_hue_to_rgb(hue, &r, &g, &b);
      rgb_write_pixel(rgb_frame, i, r, g, b);
    }
    break;
  default:
    return;
  }
  frame_dirty = true;
}

static bool rgb_store_config(const eeconfig_rgb_t *next) {
  eeconfig_rgb_t stored = *next;

  /* Live frames are a volatile host-owned stream. Persist the last autonomous
   * effect when another live-mode setting (brightness, enabled, ...) is saved,
   * without forcing the running stream out of live mode. */
  if (stored.effect == RGB_EFFECT_LIVE) {
    stored.effect = previous_effect == RGB_EFFECT_LIVE ? RGB_EFFECT_STATIC
                                                        : previous_effect;
  }
  return EECONFIG_WRITE(rgb, &stored);
}

bool rgb_init(void) {
  rgb_config = eeconfig->rgb;
  if (!rgb_effect_is_valid(rgb_config.effect) ||
      rgb_config.effect == RGB_EFFECT_LIVE)
    rgb_config.effect = RGB_EFFECT_STATIC;
  previous_effect = rgb_config.effect;
  effect_phase = 0u;
  live_chunks_received = 0u;
  last_effect_ms = timer_read();
  rgb_render_effect();
  memcpy(rgb_live_staging, rgb_frame, sizeof(rgb_live_staging));
  return rgb_backend_init();
}

void rgb_task(void) {
  rgb_backend_task();

  /* Once an off/zero-brightness frame has been submitted, freeze autonomous
   * animation until output can be visible again. This avoids blank WS2812 DMA
   * transfers and their interrupts every 20 ms while lighting is disabled. */
  if (rgb_config.enabled != 0u && rgb_config.brightness != 0u &&
      rgb_config.effect != RGB_EFFECT_STATIC &&
      rgb_config.effect != RGB_EFFECT_LIVE &&
      timer_elapsed(last_effect_ms) >= 20u) {
    last_effect_ms = timer_read();
    effect_phase++;
    rgb_render_effect();
  }

  if (!frame_dirty || rgb_backend_is_busy())
    return;

  for (uint16_t i = 0; i < RGB_LED_COUNT; i++) {
    const uint16_t src = (uint16_t)(i * RGB_BYTES_PER_PIXEL);
    const uint16_t dst =
        (uint16_t)(rgb_physical_index(i) * RGB_BYTES_PER_PIXEL);
    for (uint8_t channel = 0; channel < RGB_BYTES_PER_PIXEL; channel++) {
      const uint32_t scaled = (uint32_t)rgb_frame[src + channel] *
                                  (uint32_t)rgb_config.brightness +
                              127u;
      rgb_output[dst + channel] =
          rgb_config.enabled ? (uint8_t)(scaled / 255u) : (uint8_t)0u;
    }
  }

  if (rgb_backend_submit(rgb_output, RGB_LED_COUNT))
    frame_dirty = false;
}

bool rgb_is_enabled(void) { return rgb_config.enabled != 0u; }

bool rgb_set_enabled(bool enabled, bool persist) {
  eeconfig_rgb_t next = rgb_config;
  next.enabled = enabled ? 1u : 0u;
  if (persist && !rgb_store_config(&next))
    return false;
  rgb_config = next;
  frame_dirty = true;
  return true;
}

uint8_t rgb_get_brightness(void) { return rgb_config.brightness; }

bool rgb_set_brightness(uint8_t brightness, bool persist) {
  eeconfig_rgb_t next = rgb_config;
  next.brightness = brightness;
  if (persist && !rgb_store_config(&next))
    return false;
  rgb_config = next;
  frame_dirty = true;
  return true;
}

uint8_t rgb_get_effect(void) { return rgb_config.effect; }

bool rgb_set_effect(uint8_t effect, bool persist) {
  if (!rgb_effect_is_valid(effect))
    return false;

  eeconfig_rgb_t next = rgb_config;
  if (effect == RGB_EFFECT_LIVE && rgb_config.effect != RGB_EFFECT_LIVE)
    previous_effect = rgb_config.effect;
  next.effect = effect;

  /* Live streaming is deliberately runtime-only: selecting it can never
   * generate flash traffic at frame rate or strand the board in live mode. */
  if (persist && effect != RGB_EFFECT_LIVE && !rgb_store_config(&next))
    return false;
  rgb_config = next;
  if (effect == RGB_EFFECT_LIVE) {
    /* Entering (or explicitly restarting) live mode begins a fresh atomic
     * transaction. An incomplete upload from an earlier live session must
     * never be completed by a late chunk from a newer session. */
    live_chunks_received = 0u;
    memcpy(rgb_live_staging, rgb_frame, sizeof(rgb_live_staging));
  } else {
    effect_phase = 0u;
    last_effect_ms = timer_read();
    rgb_render_effect();
  }
  frame_dirty = true;
  return true;
}

bool rgb_restore_effect(void) {
  uint8_t effect = previous_effect;
  if (effect == RGB_EFFECT_LIVE)
    effect = RGB_EFFECT_STATIC;
  return rgb_set_effect(effect, false);
}

const uint8_t *rgb_get_frame(void) { return rgb_frame; }

bool rgb_get_pixel(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b) {
  if (index >= RGB_LED_COUNT || r == NULL || g == NULL || b == NULL)
    return false;
  *r = rgb_frame[(uint16_t)index * 3u];
  *g = rgb_frame[(uint16_t)index * 3u + 1u];
  *b = rgb_frame[(uint16_t)index * 3u + 2u];
  return true;
}

bool rgb_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (rgb_config.effect != RGB_EFFECT_LIVE || index >= RGB_LED_COUNT)
    return false;
  /* A direct pixel edit is its own publication and invalidates any incomplete
   * chunked upload, otherwise later chunks could publish a mixed frame. */
  live_chunks_received = 0u;
  rgb_write_pixel(rgb_frame, index, r, g, b);
  rgb_write_pixel(rgb_live_staging, index, r, g, b);
  frame_dirty = true;
  return true;
}

bool rgb_get_frame_chunk(uint16_t offset, uint8_t *dst, uint8_t len) {
  if (dst == NULL || len == 0u || offset >= RGB_FRAME_BYTES ||
      (uint32_t)offset + len > RGB_FRAME_BYTES)
    return false;
  memcpy(dst, rgb_frame + offset, len);
  return true;
}

bool rgb_set_frame_chunk(uint16_t offset, const uint8_t *src, uint8_t len) {
  uint16_t chunk;
  uint16_t remaining;
  uint8_t expected_len;
  const uint16_t chunk_count =
      (RGB_FRAME_BYTES + RGB_FRAME_CHUNK_BYTES - 1u) /
      RGB_FRAME_CHUNK_BYTES;
  const uint16_t required_chunks =
      (uint16_t)((1u << chunk_count) - 1u);

  if (rgb_config.effect != RGB_EFFECT_LIVE || src == NULL || len == 0u ||
      offset >= RGB_FRAME_BYTES ||
      offset % RGB_FRAME_CHUNK_BYTES != 0u)
    return false;
  chunk = offset / RGB_FRAME_CHUNK_BYTES;
  remaining = RGB_FRAME_BYTES - offset;
  expected_len = remaining > RGB_FRAME_CHUNK_BYTES
                     ? RGB_FRAME_CHUNK_BYTES
                     : (uint8_t)remaining;
  if (len != expected_len)
    return false;

  /* Chunk zero starts a new transaction. The displayed frame changes only
   * after every chunk has arrived, so a dropped/out-of-order upload never
   * exposes a mixture of old and new pixels to DMA. */
  if (chunk == 0u)
    live_chunks_received = 0u;
  memcpy(rgb_live_staging + offset, src, len);
  live_chunks_received |= (uint16_t)(1u << chunk);
  if (live_chunks_received == required_chunks) {
    memcpy(rgb_frame, rgb_live_staging, sizeof(rgb_frame));
    /* Close the transaction before accepting another chunk. In particular, a
     * late non-zero chunk must not republish a mixture of two host frames. */
    live_chunks_received = 0u;
    frame_dirty = true;
  }
  return true;
}

bool rgb_fill(uint8_t r, uint8_t g, uint8_t b, bool persist) {
  eeconfig_rgb_t next = rgb_config;

  if (rgb_config.effect != RGB_EFFECT_LIVE) {
    next.color_r = r;
    next.color_g = g;
    next.color_b = b;
    if (persist && !rgb_store_config(&next))
      return false;
    rgb_config = next;

    /* In autonomous modes FILL updates the effect's base color. Re-render the
     * selected effect instead of flashing a full-color frame until its next
     * animation tick (rainbow effects intentionally ignore the base color). */
    rgb_render_effect();
  } else {
    /* A direct fill is its own publication and cancels a partial chunked
     * transaction for the same reason as a direct pixel edit. */
    live_chunks_received = 0u;
    rgb_fill_frame(rgb_frame, r, g, b);
  }

  /* Publish the visual change only after its optional persistent transaction
   * succeeds. A failed flash write therefore leaves runtime and storage in the
   * same state. */
  memcpy(rgb_live_staging, rgb_frame, sizeof(rgb_live_staging));
  frame_dirty = true;
  return true;
}

#endif
