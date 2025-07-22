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
// Bootloader Configuration
//--------------------------------------------------------------------+

#define BOOTLOADER_ADDR 0x1FFFA400
#define BOOTLOADER_MAGIC 0xDEADBEEF

//--------------------------------------------------------------------+
// Flash Configuration
//--------------------------------------------------------------------+

// Flash size in bytes
#define FLASH_SIZE (256 * 1024)
// Number of flash sectors/pages
#define FLASH_NUM_SECTORS 128
// Flash value when erased
#define FLASH_EMPTY_VAL 0xFFFFFFFF

//--------------------------------------------------------------------+
// ADC Configuration
//--------------------------------------------------------------------+

// Number of ADC channels
#define ADC_NUM_CHANNELS 16

// ADC resolution in bits
#define ADC_RESOLUTION 12
// Maximum ADC value
#define ADC_MAX_VALUE 4095

#if !defined(ADC_NUM_SAMPLE_CYCLES)
// Number of sample cycles for each ADC conversion
#define ADC_NUM_SAMPLE_CYCLES ADC_SAMPLETIME_7_5
#endif
