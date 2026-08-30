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
#include "stm32f7xx_hal.h"

static CRC_HandleTypeDef crc_handle;

void crc32_init(void) {
  __HAL_RCC_CRC_CLK_ENABLE();

  crc_handle.Instance = CRC;
  crc_handle.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  crc_handle.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  crc_handle.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  crc_handle.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  // Unlike the F4 HAL, the F7 HAL needs an explicit input format.
  crc_handle.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
  if (HAL_CRC_Init(&crc_handle) != HAL_OK)
    board_error_handler();
}

uint32_t crc32_compute(const void *buf, uint32_t len, uint32_t crc) {
  const uint8_t *bytes = buf;
  uint32_t tail = 0;
  (void)HAL_CRC_Calculate(&crc_handle, &crc, 1);
  crc = HAL_CRC_Accumulate(&crc_handle, (uint32_t *)bytes, len >> 2);
  if ((len & 3u) != 0u) {
    memcpy(&tail, bytes + (len & ~(uint32_t)3u), len & 3u);
    crc = HAL_CRC_Accumulate(&crc_handle, &tail, 1);
  }
  return crc;
}
