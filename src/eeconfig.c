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

const eeconfig_t *eeconfig;

static const eeconfig_t default_eeconfig = {
    .magic_start = EECONFIG_MAGIC_START,
    .version = EECONFIG_VERSION,
    .calibration = DEFAULT_CALIBRATION,
    .options = DEFAULT_OPTIONS,
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
