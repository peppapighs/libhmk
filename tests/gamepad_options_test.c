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

#include <assert.h>
#include <stdio.h>

#include "eeconfig.h"
#include "migration.h"

#ifdef NDEBUG
#error "This test requires assertions"
#endif

static eeconfig_t storage;
const eeconfig_t *eeconfig = &storage;
static unsigned int write_count;
static bool write_success = true;

bool wear_leveling_write(uint32_t addr, const void *buf, uint32_t len) {
  assert(addr == 0);
  assert(len == sizeof(storage));
  write_count++;
  if (!write_success)
    return false;
  memcpy(&storage, buf, len);
  return true;
}

static void test_options_wire_layout(void) {
  const eeconfig_options_t defaults = DEFAULT_OPTIONS;
  assert(sizeof(eeconfig_options_t) == 2);
  assert(defaults.raw == 0x0004);
  assert(eeconfig_get_gamepad_api(NULL) == GAMEPAD_API_DISABLED);
  assert(!eeconfig_set_gamepad_api(NULL, GAMEPAD_API_HID));

  // Exercise every stored word, including unrelated/reserved option bits.
  for (uint32_t raw = 0; raw <= UINT16_MAX; raw++) {
    eeconfig_options_t options = {.raw = (uint16_t)raw};
    const uint32_t api = raw & 3u;
    assert((unsigned int)options.gamepad_api == api);
    assert(options.high_polling_rate_enabled == ((raw & 4u) != 0));
    assert((unsigned int)eeconfig_get_gamepad_api(&options) ==
           (api == 3u ? GAMEPAD_API_XINPUT : api));

    // Reserved 3 and values outside the 2-bit field cannot be written, and
    // rejection must leave the whole options word unchanged.
    assert(!eeconfig_set_gamepad_api(&options, (gamepad_api_t)3));
    assert(!eeconfig_set_gamepad_api(&options, (gamepad_api_t)4));
    assert(!eeconfig_set_gamepad_api(&options, (gamepad_api_t)-1));
    assert(options.raw == raw);

    for (unsigned int next = GAMEPAD_API_DISABLED; next <= GAMEPAD_API_HID;
         next++) {
      assert(eeconfig_set_gamepad_api(&options, (gamepad_api_t)next));
      assert(options.raw == ((raw & ~3u) | next));
      assert((unsigned int)eeconfig_get_gamepad_api(&options) == next);
    }
  }
}

static void prepare_v1_5(void) {
  uint8_t *bytes = (uint8_t *)&storage;
  for (size_t i = 0; i < sizeof(storage); i++)
    bytes[i] = (uint8_t)(i * 37u + 11u);
  storage.magic_start = EECONFIG_MAGIC_START;
  storage.version = 0x0105;
  storage.magic_end = EECONFIG_MAGIC_END;
  write_count = 0;
}

static void test_v1_5_migration(void) {
  // v1.5 and v1.6 have identical sizes and field offsets. Only the version
  // and former reserved bit 1 may change; calibration/profiles stay intact.
  for (uint32_t raw = 0; raw <= UINT16_MAX; raw++) {
    prepare_v1_5();
    storage.options.raw = (uint16_t)raw;
    eeconfig_t expected = storage;
    expected.version = EECONFIG_VERSION;
    expected.options.raw = (uint16_t)(raw & ~2u);

    assert(migration_try_migrate());
    assert(write_count == 1);
    assert(memcmp(&storage, &expected, sizeof(storage)) == 0);
    assert(eeconfig_get_gamepad_api(&storage.options) ==
           ((raw & 1u) ? GAMEPAD_API_XINPUT : GAMEPAD_API_DISABLED));

    // Once migrated, no second migration may reinterpret a valid HID mode.
    assert(eeconfig_set_gamepad_api(&storage.options, GAMEPAD_API_HID));
    expected = storage;
    assert(!migration_try_migrate());
    assert(write_count == 1);
    assert(memcmp(&storage, &expected, sizeof(storage)) == 0);
  }
}

static void test_migration_failure(void) {
  prepare_v1_5();
  storage.version = EECONFIG_VERSION + 1;
  eeconfig_t expected = storage;
  assert(!migration_try_migrate());
  assert(write_count == 0);
  assert(memcmp(&storage, &expected, sizeof(storage)) == 0);

  prepare_v1_5();
  expected = storage;
  write_success = false;
  assert(!migration_try_migrate());
  assert(write_count == 1);
  assert(memcmp(&storage, &expected, sizeof(storage)) == 0);
  write_success = true;
}

int main(void) {
  test_options_wire_layout();
  test_v1_5_migration();
  test_migration_failure();
  puts("gamepad options: all 65536 wire values and v1.5 migrations passed");
  return 0;
}
