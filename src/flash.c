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

uint32_t flash_sector_size(uint32_t sector) {
#if defined(FLASH_SECTOR_SIZE)
  return sector < FLASH_NUM_SECTORS ? FLASH_SECTOR_SIZE : 0;
#elif defined(FLASH_SECTOR_SIZES)
  static const uint32_t flash_sector_sizes[] = FLASH_SECTOR_SIZES;
  return sector < FLASH_NUM_SECTORS ? flash_sector_sizes[sector] : 0;
#else
#error "FLASH_UNIFORM_SECTOR_SIZE or FLASH_SECTOR_SIZES must be defined"
#endif
}