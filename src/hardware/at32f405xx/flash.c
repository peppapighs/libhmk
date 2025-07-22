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

#include "hardware/hardware.h"

#include "at32f402_405.h"

#define FLASH_SECTOR_SIZE 2048

void flash_init(void) {}

uint32_t flash_sector_size(uint32_t sector) {
  return sector < FLASH_NUM_SECTORS ? FLASH_SECTOR_SIZE : 0;
}

bool flash_erase(uint32_t sector) {
  if (sector >= FLASH_NUM_SECTORS)
    return false;

  flash_unlock();
  bool success =
      (flash_operation_wait_for(ERASE_TIMEOUT) != FLASH_OPERATE_TIMEOUT);
  if (!success) {
    flash_lock();
    return false;
  }
  flash_flag_clear(FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG);
  success = (flash_sector_erase(FLASH_BASE + sector * FLASH_SECTOR_SIZE) ==
             FLASH_OPERATE_DONE);
  flash_lock();

  return success;
}

bool flash_read(uint32_t addr, void *buf, uint32_t len) {
  if (addr + len * 4 > FLASH_SIZE)
    return false;

  memcpy(buf, (void *)(FLASH_BASE + addr), len * 4);

  return true;
}

bool flash_write(uint32_t addr, const void *buf, uint32_t len) {
  if (addr + len * 4 > FLASH_SIZE)
    return false;

  const uint32_t *buf32 = buf;

  flash_unlock();
  for (uint32_t i = 0; i < len; i++) {
    if (flash_word_program(FLASH_BASE + addr + i * 4, buf32[i]) !=
        FLASH_OPERATE_DONE) {
      flash_lock();
      return false;
    }
  }
  flash_lock();

  return true;
}
