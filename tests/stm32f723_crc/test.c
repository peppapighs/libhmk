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

// Host-only contract test; this does not simulate the F7 peripheral.
// See README.md in this directory for the native compiler command.

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "crc32.h"
#include "stm32f7xx_hal.h"

#ifdef NDEBUG
#error "This test requires assertions"
#endif

static uint32_t crc_value;
static unsigned initialized;

void board_error_handler(void) { abort(); }

HAL_StatusTypeDef HAL_CRC_Init(CRC_HandleTypeDef *handle) {
  assert(handle->Instance == CRC);
  assert(handle->Init.DefaultPolynomialUse == DEFAULT_POLYNOMIAL_ENABLE);
  assert(handle->Init.DefaultInitValueUse == DEFAULT_INIT_VALUE_ENABLE);
  assert(handle->Init.InputDataInversionMode == CRC_INPUTDATA_INVERSION_NONE);
  assert(handle->Init.OutputDataInversionMode == CRC_OUTPUTDATA_INVERSION_DISABLE);
  assert(handle->InputDataFormat == CRC_INPUTDATA_FORMAT_WORDS);
  initialized++;
  return HAL_OK;
}

static uint32_t word_crc(uint32_t state, uint32_t word) {
  state ^= word;
  for (unsigned bit = 0; bit < 32; bit++)
    state = (state << 1) ^ ((state & 0x80000000u) ? 0x04c11db7u : 0u);
  return state;
}

uint32_t HAL_CRC_Accumulate(CRC_HandleTypeDef *handle, uint32_t *words,
                          uint32_t len) {
  assert(initialized == 1);
  assert(handle->InputDataFormat == CRC_INPUTDATA_FORMAT_WORDS);
  for (uint32_t i = 0; i < len; i++)
    crc_value = word_crc(crc_value, words[i]);
  return crc_value;
}

uint32_t HAL_CRC_Calculate(CRC_HandleTypeDef *handle, uint32_t *words,
                         uint32_t len) {
  crc_value = UINT32_MAX;
  return HAL_CRC_Accumulate(handle, words, len);
}

int main(void) {
  const uint32_t words[] = {0x04030201, 0x08070605, 0x0c0b0a09};
  const uint8_t *bytes = (const uint8_t *)words;
  crc32_init();
  for (uint32_t len = 0; len <= sizeof(words); len++) {
    uint32_t expected = word_crc(UINT32_MAX, 0x12345678);
    for (uint32_t i = 0; i < len; i += 4) {
      uint32_t word = 0;
      const uint32_t remain = len - i;
      memcpy(&word, bytes + i, remain < 4 ? remain : 4);
      expected = word_crc(expected, word);
    }
    assert(crc32_compute(words, len, 0x12345678) == expected);
  }
  return 0;
}
