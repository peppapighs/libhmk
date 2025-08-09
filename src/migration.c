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

#include "migration.h"

#include "eeconfig.h"
#include "wear_leveling.h"

// `memcpy` and move pointers forward
#define MIGRATION_MEMCPY(dst, src, size)                                       \
  memcpy((dst), (src), (size)), (dst) += (size), (src) += (size)
// `memset` and move pointers forward
#define MIGRATION_MEMSET(dst, value, size)                                     \
  memset((dst), (value), (size)), (dst) += (size)
// Assign a value and move the pointer forward
#define MIGRATION_ASSIGN(dst, value, type)                                     \
  *(type *)(dst) = (value), (dst) += sizeof(type)

static bool v1_1_global_config_func(uint8_t *dst, const uint8_t *src);
static bool v1_1_profile_config_func(uint8_t profile, uint8_t *dst,
                                     const uint8_t *src);

// Migration metadata for each configuration version. The first entry is
// reserved for the initial version (v1.0) which does not require migration.
static const migration_t migrations[] = {
    {
        .version = 0x0100,
        .global_config_size = 12,
        .profile_config_size = NUM_LAYERS * NUM_KEYS    // Keymap
                               + NUM_KEYS * 4           // Actuation map
                               + NUM_ADVANCED_KEYS * 12 // Advanced keys
                               + 1                      // Tick rate
        ,
    },
    {
        .version = 0x0101,
        .global_config_size = 14,
        .profile_config_size = NUM_LAYERS * NUM_KEYS    // Keymap
                               + NUM_KEYS * 4           // Actuation map
                               + NUM_ADVANCED_KEYS * 12 // Advanced keys
                               + NUM_KEYS               // Gamepad buttons
                               + 9                      // Gamepad options
                               + 1                      // Tick rate
        ,
        .global_config_func = v1_1_global_config_func,
        .profile_config_func = v1_1_profile_config_func,
    },
};

bool migration_try_migrate(void) {
  if (eeconfig->magic_start != EECONFIG_MAGIC_START)
    // The magic start is always the same for any version.
    return false;

  const uint16_t config_version = eeconfig->version;
  // We alternate between two buffers to save the memory.
  uint8_t current_buf = 0;
  uint8_t bufs[2][sizeof(eeconfig_t)];

  // Let `bufs[0]` be the current configuration.
  memcpy(bufs[0], eeconfig, sizeof(eeconfig_t));
  // Skip v1.0 migration since it is the initial version
  for (uint32_t i = 1; i < M_ARRAY_SIZE(migrations); i++) {
    const migration_t *m = &migrations[i];
    const migration_t *prev_m = &migrations[i - 1];

    if (m->version <= config_version)
      // Skip migrations that are not applicable
      continue;

    const uint8_t *src = bufs[current_buf];
    uint8_t *dst = bufs[current_buf ^ 1];

    if (m->global_config_func && !m->global_config_func(dst, src))
      // Migration failed for the global configuration
      return false;

    if (m->profile_config_func) {
      for (uint8_t p = 0; p < NUM_PROFILES; p++) {
        // Move the pointers to the start of each profile configuration
        const uint8_t *profile_src =
            src + prev_m->global_config_size + p * prev_m->profile_config_size;
        uint8_t *profile_dst =
            dst + m->global_config_size + p * m->profile_config_size;

        if (!m->profile_config_func(p, profile_dst, profile_src))
          // Migration failed for the profile configuration
          return false;
      }
    }

    // Update the version in the destination buffer
    ((eeconfig_t *)dst)->version = m->version;
    // Switch to the next buffer for the next migration
    current_buf ^= 1;
  }

  // Make sure the configuration is valid after migration
  ((eeconfig_t *)bufs[current_buf])->magic_end = EECONFIG_MAGIC_END;
  // We reflect the update in the flash.
  return wear_leveling_write(0, &bufs[current_buf], sizeof(eeconfig_t));
}

//--------------------------------------------------------------------+
// v1.0 -> v1.1 Migration
//--------------------------------------------------------------------+

bool v1_1_global_config_func(uint8_t *dst, const uint8_t *src) {
  if (((eeconfig_t *)src)->version != 0x0100)
    // Expected version v1.0
    return false;

  // Copy `magic_start` to `calibration`
  MIGRATION_MEMCPY(dst, src, 10);
  // Default `options` to 0
  MIGRATION_ASSIGN(dst, 0, uint16_t);
  // Copy `current_profile` and `last_non_default_profile`
  MIGRATION_MEMCPY(dst, src, 2);

  return true;
}

bool v1_1_profile_config_func(uint8_t profile, uint8_t *dst,
                              const uint8_t *src) {
  // Save the `keymap` offset
  uint8_t *keymap = dst;
  // Copy `keymap` to `actuation_map`
  MIGRATION_MEMCPY(dst, src, (NUM_LAYERS * NUM_KEYS) + (NUM_KEYS * 4));
  // Update keycodes to include `KC_INT1` ... `KC_LNG6`
  for (uint32_t i = 0; i < NUM_LAYERS * NUM_KEYS; i++) {
    if (0x70 <= keymap[i] && keymap[i] <= 0x71)
      // `KC_LNG1` and `KC_LNG2`
      keymap[i] += 0x05;
    else if (0x72 <= keymap[i] && keymap[i] <= 0x96)
      // `KC_LEFT_CTRL` ... `SP_MOUSE_BUTTON_5`
      keymap[i] += 0x0A;
  }
  // Save the `advanced_keys` offset
  uint8_t *advanced_keys = dst;
  // Copy `advanced_keys`
  MIGRATION_MEMCPY(dst, src, NUM_ADVANCED_KEYS * 12);
  // Default `hold_on_other_key_press` to 0
  for (uint8_t i = 0; i < NUM_ADVANCED_KEYS; i++) {
    uint8_t *ak = advanced_keys + i * 12;
    if (ak[2] == AK_TYPE_TAP_HOLD)
      ak[7] = 0;
  }
  // Set `gamepad_buttons` to 0
  MIGRATION_MEMSET(dst, 0, NUM_KEYS);
  // Default `analog_curve` to linear
  MIGRATION_ASSIGN(dst, 4, uint8_t), MIGRATION_ASSIGN(dst, 20, uint8_t);
  MIGRATION_ASSIGN(dst, 85, uint8_t), MIGRATION_ASSIGN(dst, 95, uint8_t);
  MIGRATION_ASSIGN(dst, 165, uint8_t), MIGRATION_ASSIGN(dst, 170, uint8_t);
  MIGRATION_ASSIGN(dst, 255, uint8_t), MIGRATION_ASSIGN(dst, 255, uint8_t);
  // Default `keyboard_enabled` and `snappy_joystick` to true
  MIGRATION_ASSIGN(dst, 0b00001001, uint8_t);
  // Copy `tick_rate`
  MIGRATION_MEMCPY(dst, src, 1);

  return true;
}
