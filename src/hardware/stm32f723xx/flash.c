/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "hardware/hardware.h"

#include "stm32f7xx_hal.h"

void flash_init(void) {}

bool flash_erase(uint32_t sector) {
  if (sector >= FLASH_NUM_SECTORS)
    return false;

  FLASH_EraseInitTypeDef erase = {0};
  uint32_t error = 0;
  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.Sector = sector;
  erase.NbSectors = 1;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  HAL_FLASH_Unlock();
  const bool success = HAL_FLASHEx_Erase(&erase, &error) == HAL_OK;
  HAL_FLASH_Lock();
  return success;
}

bool flash_read(uint32_t addr, void *buf, uint32_t len) {
  if (addr > FLASH_SIZE || len > (FLASH_SIZE - addr) / sizeof(uint32_t))
    return false;
  memcpy(buf, (const void *)(FLASH_BASE + addr), len * sizeof(uint32_t));
  return true;
}

bool flash_write(uint32_t addr, const void *buf, uint32_t len) {
  if (addr > FLASH_SIZE || len > (FLASH_SIZE - addr) / sizeof(uint32_t))
    return false;

  const uint32_t *words = buf;
  HAL_FLASH_Unlock();
  for (uint32_t i = 0; i < len; i++) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                          FLASH_BASE + addr + i * sizeof(uint32_t),
                          words[i]) != HAL_OK) {
      HAL_FLASH_Lock();
      return false;
    }
  }
  HAL_FLASH_Lock();
  return true;
}
