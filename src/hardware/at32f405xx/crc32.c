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

#include "at32f402_405.h"
#include "hardware/hardware.h"

void crc32_init(void) { crm_periph_clock_enable(CRM_CRC_PERIPH_CLOCK, TRUE); }

uint32_t crc32_compute(const void *buf, uint32_t len, uint32_t crc) {
  const uint8_t *buf8 = buf;
  uint32_t k = 0;

  crc_data_reset();
  crc_one_word_calculate(crc);
  crc = crc_block_calculate((uint32_t *)buf8, len >> 2);
  if (len & 3) {
    memcpy(&k, buf8 + (len & ~3), len & 3);
    crc = crc_one_word_calculate(k);
  }

  return crc;
}
