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

//--------------------------------------------------------------------+
// ADC Configuration
//--------------------------------------------------------------------+

#if !defined(ADC_NUM_SAMPLE_CYCLES)
// Number of sample cycles for each ADC conversion
#define ADC_NUM_SAMPLE_CYCLES ADC_SAMPLETIME_3CYCLES
#endif

// ADC resolution in bits, set by `scripts/make.py`
#if ADC_RESOLUTION == 12
#define ADC_RESOLUTION_HAL ADC_RESOLUTION_12B
#elif ADC_RESOLUTION == 10
#define ADC_RESOLUTION_HAL ADC_RESOLUTION_10B
#elif ADC_RESOLUTION == 8
#define ADC_RESOLUTION_HAL ADC_RESOLUTION_8B
#elif ADC_RESOLUTION == 6
#define ADC_RESOLUTION_HAL ADC_RESOLUTION_6B
#else
#error "Unsupported ADC resolution"
#endif
