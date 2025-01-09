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

#include "config.h"

//--------------------------------------------------------------------+
// Switch Configuration
//--------------------------------------------------------------------+

#if !defined(ADC_RESOLUTION)
// The resolution of the ADC in bits
#define ADC_RESOLUTION 12
#endif

#if !defined(CALIBRATION_TIME)
// The time in milliseconds to wait for the switch calibration to finish
#define CALIBRATION_TIME 500
#endif

#if !defined(SMOOTHING_FACTOR_RECIPROCAL_LOG2)
// log2 of the reciprocal of the smoothing factor e.g. 4 for 1/16
#define SMOOTHING_FACTOR_RECIPROCAL_LOG2 4
#endif

//--------------------------------------------------------------------+
// Switch Macros
//--------------------------------------------------------------------+

// Add two ADC values and saturate at the maximum ADC value
#define SAFE_ADC_ADD(a, b)                                                     \
    ((uint32_t)(a) + (b) < (1 << ADC_RESOLUTION) ? (a) + (b)                   \
                                                 : (1 << ADC_RESOLUTION) - 1)

//--------------------------------------------------------------------+
// Switch Types
//--------------------------------------------------------------------+

typedef struct {
    // Current ADC value of the switch
    uint16_t adc_value;
    // Minimum ADC value sampled during calibration
    uint16_t min_adc_value;
    // Maximum ADC value observed during runtime
    uint16_t max_adc_value;
} adc_state_t;

enum {
    SW_DIR_NONE = 0,
    SW_DIR_UP,
    SW_DIR_DOWN,
};

typedef struct {
    adc_state_t adc_state;

    // Travel distance of the switch in 0.05mm
    uint16_t distance;
    uint16_t peek_distance;

    // Only used by rapid trigger and dynamic keystroke
    uint8_t sw_dir;
    // 0: Not pressed, 1: Pressed
    uint8_t sw_state;
} switch_state_t;

// Supported magnetic switches
enum {
    // https://geon.works/products/geon-raw-he-switch
    SW_GEON_RAW_HE = 0,
    // https://www.gateron.com/products/gateron-magic-jade-switch
    SW_GATERON_MAGNETIC_JADE,
    // https://geon.works/products/raptor-he-gaming-switch
    SW_GEON_RAPTOR_HE,
    SW_COUNT,
};

// Travel distance of the supported switches in 0.05mm
extern const uint16_t sw_distance[];

//--------------------------------------------------------------------+
// Switch APIs
//--------------------------------------------------------------------+

/**
 * @brief Initialize the switch module
 *
 * This function handles the startup of the switch calibration.
 *
 * @return none
 */
void switch_init(void);

/**
 * @brief Wait for the switch calibration to finish
 *
 * @return none
 */
void wait_for_switch_calibration(void);

/**
 * @brief Get the initial minimum ADC value of the switch
 *
 * This function must be implemented by the user. It should return a value as
 * close as possible to the ADC value of an unpressed switch. A value too high
 * may require a slightly longer calibration time. A value too low may cause a
 * dead zone at the top of the switch.
 *
 * @param sw_id The ID of the switch e.g. `SW_GEON_RAW_HE`
 *
 * @return The initial minimum ADC value of the switch
 *
 */
uint16_t calibration_initial_min(uint8_t sw_id);

/**
 * @brief Get the delta value of the switch
 *
 * This function must be implemented by the user. It should return a value as
 * close as possible to the difference between the ADC value of a pressed and
 * unpressed switch. A value too high may may cause a dead zone at the bottom of
 * the switch. A value too low may cause the travel distance calculation to be
 * inaccurate until the first bottom-out.
 *
 * @param sw_id The ID of the switch e.g. `SW_GEON_RAW_HE`
 *
 * @return The delta value of the switch
 */
uint16_t calibration_delta(uint8_t sw_id);

/**
 * @brief Recalibrate the switches
 *
 * @return none
 */
void switch_recalibrate(void);

/**
 * @brief Store the ADC value of the switch
 *
 * This function should be called by the ADC module whenever a new ADC value is
 * available and can be safely called in an interrupt context. The ADC
 * exponential smoothing is performed here. If the calibration is not finished,
 * the ADC value will also be used to calibrate the switch.
 *
 * @param index The switch index
 * @param adc_value The ADC value of the switch
 *
 * @return none
 */
void store_adc_value(uint8_t index, uint16_t adc_value);

/**
 * @brief Calculate the switch travel distance from the ADC value of the switch
 *
 * This function will be called by the `matrix_scan` function. A default
 * implementation is provided but can be overridden by the user.
 *
 * @param adc_state The ADC state of the switch
 * @param sw_id The ID of the switch e.g. `SW_GEON_RAW_HE`
 * @param sw_dist The travel distance of the switch in 0.05mm
 *
 * @return The travel distance of the switch in 0.05mm
 */
uint8_t adc_state_to_distance(const adc_state_t *adc_state, uint8_t sw_id,
                              uint8_t sw_dist);

/**
 * @brief Scan the switches
 *
 * This function should be called in the main loop. It will calculate the switch
 * travel distance from the ADC value and update the switch state.
 *
 * @return none
 */
void matrix_scan(void);

/**
 * @brief Get the ADC value of the switch
 *
 * @param index The switch index
 *
 * @return The ADC value of the switch
 */
uint16_t get_switch_adc_value(uint8_t index);

/**
 * @brief Get the switch distance
 *
 * @param index The switch index
 *
 * @return The switch distance
 */
uint8_t get_switch_distance(uint8_t index);

/**
 * @brief Get the switch state
 *
 * @param index The switch index
 *
 * @return The switch state
 */
uint8_t get_switch_state(uint8_t index);
