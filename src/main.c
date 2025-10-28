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

#include "advanced_keys.h"
#include "commands.h"
#include "crc32.h"
#include "deferred_actions.h"
#include "eeconfig.h"
#include "hardware/hardware.h"
#include "hid.h"
#include "layout.h"
#include "matrix.h"
#include "tusb.h"
#include "wear_leveling.h"
#include "xinput.h"

int main(void) {
  // Initialize the hardware
  board_init();
  timer_init();
  crc32_init();
  flash_init();

  // Initialize the persistent configuration
  wear_leveling_init();
  eeconfig_init();

  // Initialize the core modules
  analog_init();
  matrix_init();
  hid_init();
  deferred_action_init();
  advanced_key_init();
  xinput_init();
  layout_init();
  command_init();

  tud_init(BOARD_TUD_RHPORT);

  while (1) {
    tud_task();

    analog_task();
    matrix_scan();
    layout_task();
    xinput_task();
  }

  return 0;
}
