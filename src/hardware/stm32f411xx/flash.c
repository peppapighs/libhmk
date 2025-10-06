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

#include "stm32f4xx_hal.h"

static const uint32_t flash_sector_sizes[] = {
    16 * 1024, 16 * 1024,  16 * 1024,  16 * 1024,
    64 * 1024, 128 * 1024, 128 * 1024, 128 * 1024,
};

_Static_assert(M_ARRAY_SIZE(flash_sector_sizes) == FLASH_NUM_SECTORS,
               "Invalid number of flash sectors");

void flash_init(void) {}

uint32_t flash_sector_size(uint32_t sector) {
  return sector < FLASH_NUM_SECTORS ? flash_sector_sizes[sector] : 0;
}

bool flash_erase(uint32_t sector) {
  if (sector >= FLASH_NUM_SECTORS)
    return false;

  FLASH_EraseInitTypeDef erase = {0};
  uint32_t error = 0;
  bool success = true;

  HAL_FLASH_Unlock();
  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.Banks = FLASH_BANK_1;
  erase.Sector = sector;
  erase.NbSectors = 1;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  success = (HAL_FLASHEx_Erase(&erase, &error) == HAL_OK);
  HAL_FLASH_Lock();

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

  HAL_FLASH_Unlock();
  for (uint32_t i = 0; i < len; i++) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_BASE + addr + i * 4,
                          buf32[i]) != HAL_OK) {
      HAL_FLASH_Lock();
      return false;
    }
  }
  HAL_FLASH_Lock();

  return true;
}
