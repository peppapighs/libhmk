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
// Bitmap Types
//--------------------------------------------------------------------+

typedef uint32_t bitmap_t;

//--------------------------------------------------------------------+
// Bitmap API
//--------------------------------------------------------------------+

/**
 * @brief Create a bitmap of a given length
 *
 * @param len Length of the bitmap in bits
 *
 * @return Bitmap
 */
#define MAKE_BITMAP(len) ((bitmap_t[M_DIV_CEIL(len, 32)]){0})

/**
 * @brief Get the value of a bit in a bitmap
 *
 * @param bitmap Bitmap
 * @param i Index of the bit
 *
 * @return Value of the bit
 */
__attribute__((always_inline)) static inline uint8_t
bitmap_get(const bitmap_t *bitmap, uint32_t i) {
  return (bitmap[i / 32] >> (i & 31)) & 1;
}

/**
 * @brief Set the value of a bit in a bitmap
 *
 * @param bitmap Bitmap
 * @param i Index of the bit
 * @param v Value to set
 *
 * @return None
 */
__attribute__((always_inline)) static inline void
bitmap_set(bitmap_t *bitmap, uint32_t i, uint8_t v) {
  bitmap[i / 32] ^= (uint32_t)(bitmap_get(bitmap, i) ^ v) << (i & 31);
}

/**
 * @brief Toggle the value of a bit in a bitmap
 *
 * @param bitmap Bitmap
 * @param i Index of the bit
 *
 * @return None
 */
__attribute__((always_inline)) static inline void
bitmap_toggle(bitmap_t *bitmap, uint32_t i) {
  bitmap[i / 32] ^= 1UL << (i & 31);
}
