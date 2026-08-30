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

#pragma once
#include <stdint.h>

#define __HAL_RCC_CRC_CLK_ENABLE() ((void)0)
#define CRC ((void *)0x40023000)
#define DEFAULT_POLYNOMIAL_ENABLE 0u
#define DEFAULT_INIT_VALUE_ENABLE 0u
#define CRC_INPUTDATA_INVERSION_NONE 0u
#define CRC_OUTPUTDATA_INVERSION_DISABLE 0u
#define CRC_INPUTDATA_FORMAT_WORDS 3u

typedef int HAL_StatusTypeDef;
#define HAL_OK 0

typedef struct {
  void *Instance;
  struct {
    uint8_t DefaultPolynomialUse;
    uint8_t DefaultInitValueUse;
    uint32_t InputDataInversionMode;
    uint32_t OutputDataInversionMode;
  } Init;
  uint32_t InputDataFormat;
} CRC_HandleTypeDef;

HAL_StatusTypeDef HAL_CRC_Init(CRC_HandleTypeDef *handle);
uint32_t HAL_CRC_Calculate(CRC_HandleTypeDef *handle, uint32_t *words, uint32_t len);
uint32_t HAL_CRC_Accumulate(CRC_HandleTypeDef *handle, uint32_t *words, uint32_t len);
