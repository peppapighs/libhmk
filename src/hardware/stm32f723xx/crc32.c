/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "crc32.h"

#include "hardware/hardware.h"
#include "stm32f7xx_hal.h"

static CRC_HandleTypeDef crc_handle;

void crc32_init(void) {
  __HAL_RCC_CRC_CLK_ENABLE();
  crc_handle.Instance = CRC;
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
