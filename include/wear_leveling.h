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

#include "common.h"

//--------------------------------------------------------------------+
// Wear Leveling Configuration
//--------------------------------------------------------------------+

#if !defined(WL_VIRTUAL_SIZE)
// Size of the virtual persistent storage in bytes. There must be enough RAM of
// this size to hold the entire virtual storage.
#define WL_VIRTUAL_SIZE 8192
#endif

_Static_assert(WL_VIRTUAL_SIZE <= 8192,
               "WL_VIRTUAL_SIZE must be at most 8192.");
_Static_assert(WL_VIRTUAL_SIZE % 4 == 0,
               "WL_VIRTUAL_SIZE must be word-aligned.");

#if !defined(WL_WRITE_LOG_SIZE)
// Size of the write log in bytes
#define WL_WRITE_LOG_SIZE 65536
#endif

_Static_assert(WL_WRITE_LOG_SIZE % 4 == 0,
               "WL_WRITE_LOG_SIZE must be word-aligned.");

// Flash space in bytes used by the wear leveling module
#define WL_BACKING_STORE_SIZE (WL_VIRTUAL_SIZE + WL_WRITE_LOG_SIZE)

_Static_assert(WL_BACKING_STORE_SIZE <= FLASH_SIZE,
               "The wear leveling backing store must fit in flash.");

//--------------------------------------------------------------------+
// Wear Leveling Write Log Entry
//--------------------------------------------------------------------+

#define WL_LOG_ENTRY_SIZE 8
#define WL_MAX_BYTES_PER_ENTRY 6

typedef union __attribute__((packed)) {
  struct __attribute__((packed)) {
    uint16_t addr : 13;
    uint8_t len : 3;
    uint8_t data[WL_MAX_BYTES_PER_ENTRY];
  } fields;
  uint32_t raw[2];
} wl_log_entry_t;

_Static_assert(sizeof(wl_log_entry_t) == WL_LOG_ENTRY_SIZE,
               "wl_log_entry_t must be 8 bytes.");

//--------------------------------------------------------------------+
// Wear Leveling Cache
//--------------------------------------------------------------------+

// The virtual storage of the wear leveling module. It is automatically
// updated when the wear leveling module is initialized and a write operation
// is performed. Do not modify this array directly.
extern uint8_t wl_cache[];

//--------------------------------------------------------------------+
// Wear Leveling API
//--------------------------------------------------------------------+

/**
 * @brief Initialize the wear leveling module
 *
 * @return None
 */
void wear_leveling_init(void);

/**
 * @brief Erase the virtual storage
 *
 * @return true if the erase was successful, false otherwise
 */
bool wear_leveling_erase(void);

/**
 * @brief Read data from the virtual storage
 *
 * @param addr Address to read from
 * @param buf Buffer to read into
 * @param len Length of the data in bytes
 *
 * @return true if the read was successful, false otherwise
 */
bool wear_leveling_read(uint32_t addr, void *buf, uint32_t len);

/**
 * @brief Write data to the virtual storage
 *
 * @param addr Address to write to
 * @param buf Buffer to write from
 * @param len Length of the data in bytes
 *
 * @return true if the write was successful, false otherwise
 */
bool wear_leveling_write(uint32_t addr, const void *buf, uint32_t len);
