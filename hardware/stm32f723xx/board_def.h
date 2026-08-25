/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#pragma once

/* TinyUSB root port 1 is the F723's DWC2 high-speed controller with its
 * integrated PHY (PB14/PB15). */
#define CFG_TUSB_RHPORT1_MODE (OPT_MODE_DEVICE | OPT_MODE_HIGH_SPEED)

#if !defined(ADC_NUM_SAMPLE_CYCLES)
#define ADC_NUM_SAMPLE_CYCLES ADC_SAMPLETIME_3CYCLES
#endif

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
