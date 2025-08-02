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

#include "advanced_keys.h"
#include "keycodes.h"
#include "layout.h"
#include "migration.h"

// Helper macro to update a field in the configuration
#define EECONFIG_UPDATE(field, value)                                          \
  wear_leveling_write(offsetof(eeconfig_t, field), value,                      \
                      sizeof(((eeconfig_t *)0)->field))
#define EECONFIG_UPDATE_N(field, value, len)                                   \
  wear_leveling_write(offsetof(eeconfig_t, field), value, len)

const eeconfig_t *eeconfig;

static const eeconfig_t default_eeconfig = {
    .magic_start = EECONFIG_MAGIC_START,
    .version = EECONFIG_VERSION,
    .calibration = DEFAULT_CALIBRATION,
    .current_profile = 0,
    .last_non_default_profile = M_MIN(1, NUM_PROFILES - 1),
    .profiles = {[0 ... NUM_PROFILES - 1] = DEFAULT_PROFILE},
    .magic_end = EECONFIG_MAGIC_END,
};

static bool eeconfig_is_latest_version(void) {
  return eeconfig->magic_start == EECONFIG_MAGIC_START &&
         eeconfig->magic_end == EECONFIG_MAGIC_END &&
         eeconfig->version == EECONFIG_VERSION;
}

void eeconfig_init(void) {
  eeconfig = (const eeconfig_t *)wl_cache;
  if (!eeconfig_is_latest_version() && !migration_try_migrate())
    eeconfig_reset();
}

bool eeconfig_reset(void) {
  const bool status =
      wear_leveling_write(0, &default_eeconfig, sizeof(default_eeconfig));
  layout_load_advanced_keys();

  return status;
}

bool eeconfig_set_tick_rate(uint8_t profile, uint8_t tick_rate) {
  if (profile >= NUM_PROFILES)
    return false;

  return EECONFIG_UPDATE(profiles[profile].tick_rate, &tick_rate);
}

bool eeconfig_set_calibration(const void *calibration) {
  return EECONFIG_UPDATE(calibration, calibration);
}

bool eeconfig_set_options(const void *options) {
  return EECONFIG_UPDATE(options, options);
}

bool eeconfig_set_current_profile(uint8_t profile) {
  if (profile >= NUM_PROFILES)
    return false;

  advanced_key_clear();
  bool status = EECONFIG_UPDATE(current_profile, &profile);
  if (status && profile != 0)
    status = EECONFIG_UPDATE(last_non_default_profile, &profile);
  layout_load_advanced_keys();

  return status;
}

bool eeconfig_set_keymap(uint8_t profile, uint8_t layer, uint8_t start,
                         uint8_t len, const void *keymap) {
  if (profile >= NUM_PROFILES || layer >= NUM_LAYERS || start + len > NUM_KEYS)
    return false;

  return EECONFIG_UPDATE_N(profiles[profile].keymap[layer][start], keymap, len);
}

bool eeconfig_set_actuation_map(uint8_t profile, uint8_t start, uint8_t len,
                                const void *actuation_map) {
  if (profile >= NUM_PROFILES || start + len > NUM_KEYS)
    return false;

  return EECONFIG_UPDATE_N(profiles[profile].actuation_map[start],
                           actuation_map, len * sizeof(actuation_t));
}

bool eeconfig_set_advanced_keys(uint8_t profile, uint8_t start, uint8_t len,
                                const void *advanced_key) {
  if (profile >= NUM_PROFILES || start + len > NUM_ADVANCED_KEYS)
    return false;

  if (eeconfig->current_profile == profile)
    // We only need to clear the advanced keys if the advanced keys in
    // the current profile are being updated.
    advanced_key_clear();
  const bool status =
      EECONFIG_UPDATE_N(profiles[profile].advanced_keys[start], advanced_key,
                        len * sizeof(advanced_key_t));
  if (eeconfig->current_profile == profile)
    // Same as above
    layout_load_advanced_keys();

  return status;
}
