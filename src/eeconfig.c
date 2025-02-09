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

// Helper macro to update a field in the configuration
#define EECONFIG_UPDATE(field, value)                                          \
  wear_leveling_write(offsetof(eeconfig_t, field), value,                      \
                      sizeof(((eeconfig_t *)0)->field))

const eeconfig_t *eeconfig;

static const eeconfig_t default_eeconfig = {
    .magic_start = EECONFIG_MAGIC_START,
    .version = EECONFIG_VERSION,
    .current_profile = 0,
    .last_non_default_profile = 0,
    .profiles = {[0 ... NUM_PROFILES - 1] = DEFAULT_PROFILE},
    .magic_end = EECONFIG_MAGIC_END,
};

void eeconfig_init(void) {
  eeconfig = (const eeconfig_t *)wl_cache;
  if (eeconfig->magic_start != EECONFIG_MAGIC_START ||
      eeconfig->magic_end != EECONFIG_MAGIC_END)
    // If the configuration is invalid, reset it
    eeconfig_reset();
}

bool eeconfig_reset(void) {
  advanced_key_clear();
  const bool status =
      wear_leveling_write(0, &default_eeconfig, sizeof(default_eeconfig));
  layout_load_advanced_keys();

  return status;
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

bool eeconfig_update_keymap(uint8_t profile, const void *keymap) {
  if (profile >= NUM_PROFILES)
    return false;

  return EECONFIG_UPDATE(profiles[profile].keymap, keymap);
}

bool eeconfig_update_actuation_map(uint8_t profile, const void *actuation_map) {
  if (profile >= NUM_PROFILES)
    return false;

  return EECONFIG_UPDATE(profiles[profile].actuation_map, actuation_map);
}

bool eeconfig_update_advanced_key(uint8_t profile, const void *advanced_key) {
  if (profile >= NUM_PROFILES)
    return false;

  if (eeconfig->current_profile == profile)
    // We only need to clear the advanced keys if the advanced keys in
    // the current profile are being updated.
    advanced_key_clear();
  const bool status =
      EECONFIG_UPDATE(profiles[profile].advanced_keys, advanced_key);
  if (eeconfig->current_profile == profile)
    // Same as above
    layout_load_advanced_keys();

  return status;
}
