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

#include "eeconfig.h"

#include "keycodes.h"
#include "migration.h"

const eeconfig_t *eeconfig;

// Default configuration values
static eeconfig_options_t default_options = DEFAULT_OPTIONS;
static eeconfig_calibration_t default_calibration = DEFAULT_CALIBRATION;
static eeconfig_profile_t default_profile = {
    .keymap = DEFAULT_KEYMAP,
    .gamepad_options = DEFAULT_GAMEPAD_OPTIONS,
    .tick_rate = DEFAULT_TICK_RATE,
};

static bool eeconfig_is_latest_version(void) {
  return eeconfig->magic_start == EECONFIG_MAGIC_START &&
         eeconfig->magic_end == EECONFIG_MAGIC_END &&
         eeconfig->version == EECONFIG_VERSION;
}

void eeconfig_init(void) {
  // Update default profile with its default values
  for (uint32_t i = 0; i < NUM_KEYS; i++)
    default_profile.actuation_map[i].actuation_point = DEFAULT_ACTUATION_POINT;

  eeconfig = (const eeconfig_t *)wl_cache;
  if (!eeconfig_is_latest_version() && !migration_try_migrate())
    eeconfig_reset();
}

// Helper macro for writing rvalue
#define EECONFIG_WRITE_LOCAL(field, value)                                     \
  do {                                                                         \
    typeof(((eeconfig_t *)0)->field) _value = value;                           \
    status &= EECONFIG_WRITE(field, &_value);                                  \
  } while (0)

bool eeconfig_reset(void) {
  uint16_t bottom_out_threshold[NUM_KEYS] = {0};

  // We must not perform any action here that requires reading from
  // the configuration as it may be in an invalid state.
  bool status = true;
  EECONFIG_WRITE_LOCAL(magic_start, EECONFIG_MAGIC_START);
  EECONFIG_WRITE_LOCAL(version, EECONFIG_VERSION);
  status &= EECONFIG_WRITE(calibration, &default_calibration);
  status &= EECONFIG_WRITE(bottom_out_threshold, bottom_out_threshold);
  status &= EECONFIG_WRITE(options, &default_options);
  EECONFIG_WRITE_LOCAL(current_profile, 0);
  EECONFIG_WRITE_LOCAL(last_non_default_profile, M_MIN(1, NUM_PROFILES - 1));
  for (uint32_t i = 0; i < NUM_PROFILES; i++)
    status &= EECONFIG_WRITE(profiles[i], &default_profile);
  EECONFIG_WRITE_LOCAL(magic_end, EECONFIG_MAGIC_END);

  return status;
}

#undef EECONFIG_WRITE_LOCAL

bool eeconfig_reset_profile(uint8_t profile) {
  if (profile >= NUM_PROFILES)
    return false;

  return EECONFIG_WRITE(profiles[profile], &default_profile);
}
