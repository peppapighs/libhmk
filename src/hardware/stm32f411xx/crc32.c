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

#include "crc32.h"

#include "hardware/hardware.h"
#include "stm32f4xx_hal.h"

static CRC_HandleTypeDef crc_handle;

void crc32_init(void) {
  __HAL_RCC_CRC_CLK_ENABLE();

  crc_handle.Instance = CRC;
  if (HAL_CRC_Init(&crc_handle) != HAL_OK)
    board_error_handler();
}

uint32_t crc32_compute(const void *buf, uint32_t len, uint32_t crc) {
  const uint8_t *buf8 = buf;
  uint32_t k = 0;

  HAL_CRC_Calculate(&crc_handle, &crc, 1);
  crc = HAL_CRC_Accumulate(&crc_handle, (void *)buf8, len >> 2);
  if (len & 3) {
    memcpy(&k, buf8 + (len & ~(uint32_t)3), len & 3);
    crc = HAL_CRC_Accumulate(&crc_handle, &k, 1);
  }

  return crc;
}
