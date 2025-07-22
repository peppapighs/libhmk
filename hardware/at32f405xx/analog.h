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
// Analog Configuration
//--------------------------------------------------------------------+

#if !defined(ADC_NUM_MUX_INPUTS)
// Number of ADC inputs that are connected to the multiplexer
#define ADC_NUM_MUX_INPUTS 0
#endif

#if ADC_NUM_MUX_INPUTS > 0
#if !defined(ADC_MUX_INPUT_CHANNELS)
#error "ADC_MUX_INPUT_CHANNELS is not defined"
#endif

#if !defined(ADC_NUM_MUX_SELECT_PINS)
#error "ADC_NUM_MUX_SELECT_PINS is not defined"
#endif

#if !defined(ADC_MUX_SELECT_PORTS)
#error "ADC_MUX_SELECT_PORTS is not defined"
#endif

#if !defined(ADC_MUX_SELECT_PINS)
#error "ADC_MUX_SELECT_PINS is not defined"
#endif

#if !defined(ADC_MUX_INPUT_MATRIX)
#error "ADC_MUX_INPUT_MATRIX is not defined"
#endif

#if !defined(ADC_SAMPLE_DELAY)
// Delay in microseconds to allow the multiplexer outputs to settle
#define ADC_SAMPLE_DELAY 20
#endif

_Static_assert((F_CPU / 1000000) * ADC_SAMPLE_DELAY < 65536,
               "ADC_SAMPLE_DELAY exceeds maximum timer period");
#endif

#if !defined(ADC_NUM_RAW_INPUTS)
// Number of ADC inputs that are connected directly to the keys
#define ADC_NUM_RAW_INPUTS 0
#endif

#if ADC_NUM_RAW_INPUTS > 0
#if !defined(ADC_RAW_INPUT_CHANNELS)
#error "ADC_RAW_INPUT_CHANNELS is not defined"
#endif

#if !defined(ADC_RAW_INPUT_VECTOR)
#error "ADC_RAW_INPUT_VECTOR is not defined"
#endif
#endif

#if !(0 < (ADC_NUM_MUX_INPUTS + ADC_NUM_RAW_INPUTS) &&                         \
      (ADC_NUM_MUX_INPUTS + ADC_NUM_RAW_INPUTS) <= ADC_NUM_CHANNELS)
#error "Invalid number of ADC inputs"
#endif
