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

static bool migration_v1_0_to_v1_1(void *dst, const void *src);

static const migration_func_t migration_funcs[] = {
    {.version = 0x0101, .migrate = migration_v1_0_to_v1_1},
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
  for (uint32_t i = 0; i < M_ARRAY_SIZE(migration_funcs); i++) {
    if (migration_funcs[i].version <= config_version ||
        migration_funcs[i].migrate == NULL)
      // Older versions, or no migration needed
      continue;

    const uint8_t *src = bufs[current_buf];
    uint8_t *dst = bufs[current_buf ^ 1];

    if (!migration_funcs[i].migrate(dst, src))
      // Migration failed
      return false;

    // Swap buffers for the next migration step
    current_buf ^= 1;
  }

  // Make sure the destination buffer is valid
  ((eeconfig_t *)bufs[current_buf])->magic_end = EECONFIG_MAGIC_END;

  // We reflect the update in the flash.
  wear_leveling_write(0, &bufs[current_buf], sizeof(eeconfig_t));

  return true;
}

//--------------------------------------------------------------------+
// v1.0 -> v1.1 Migration
//--------------------------------------------------------------------+

static bool migration_v1_0_to_v1_1(void *dst, const void *src) {
  if (*(uint16_t *)(src + 4) != 0x0100)
    // Not a v1.0 configuration, cannot migrate
    return false;

  const uint32_t profile_size = NUM_LAYERS * NUM_KEYS    // Keymap
                                + NUM_KEYS * 4           // Actuation map
                                + NUM_ADVANCED_KEYS * 12 // Advanced keys
                                + 1;                     // Tick rate

  // Option bitmap is added after calibration at the offset 10-11.
  memcpy(dst, src, 10);
  memcpy((uint8_t *)dst + 12, (uint8_t *)src + 10, sizeof(eeconfig_t) - 12);
  // Default all options to 0
  *(uint16_t *)(dst + 10) = 0;

  for (uint32_t i = 0; i < NUM_PROFILES; i++) {
    for (uint32_t j = 0; j < NUM_ADVANCED_KEYS; j++) {
      const uint8_t *ak = dst + 13                // Headers and options
                          + i * profile_size      // Profiles
                          + NUM_LAYERS * NUM_KEYS // Keymap
                          + NUM_KEYS * 4          // Actuation map
                          + j * 12;               // Advanced keys
      if (*(ak + 2) == AK_TYPE_TAP_HOLD)
        // Default hold on other key press to false
        *(bool *)(ak + 7) = false;
    }
  }

  // Update the version to 1.1
  ((eeconfig_t *)dst)->version = 0x0101;

  return true;
}
