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
// Key Matrix Configuration
//--------------------------------------------------------------------+

#if !defined(MATRIX_CALIBRATION_DURATION)
// Duration of the calibration process in milliseconds
#define MATRIX_CALIBRATION_DURATION 500
#endif

#if !defined(MATRIX_EMA_ALPHA_EXPONENT)
// Exponent of the alpha parameter of the exponential moving average (EMA)
// filter used to smooth the ADC values. Higher values will result in smoother
// but slower changes in the filtered ADC values. The alpha parameter is used in
// the formula: y_n = alpha * x_n + (1 - alpha) * y_{n-1}
#define MATRIX_EMA_ALPHA_EXPONENT 4
#endif

#if !defined(MATRIX_CALIBRATION_EPSILON)
// Minimum change in ADC values required to update the calibration values. This
// is used to mitigate the inconsistency of the Hall effect sensors.
#define MATRIX_CALIBRATION_EPSILON 5
#endif

#if !defined(MATRIX_INACTIVITY_TIMEOUT)
// Inactivity timeout in milliseconds. Bottom-out threshold will be saved after
// there is no change to the threshold of any key for this duration.
#define MATRIX_INACTIVITY_TIMEOUT 3000
#endif

//--------------------------------------------------------------------+
// Key Matrix
//--------------------------------------------------------------------+

typedef enum {
  KEY_DIR_INACTIVE = 0,
  KEY_DIR_DOWN,
  KEY_DIR_UP,
} key_dir_t;

// Key state
typedef struct {
  // Filtered ADC value
  uint16_t adc_filtered;
  // ADC value when the key is fully released
  uint16_t adc_rest_value;
  // ADC value when the key is fully pressed
  uint16_t adc_bottom_out_value;

  // Key travel distance (0-255)
  uint8_t distance;
  // Last extremum point of the key travel distance (0-255)
  uint8_t extremum;
  // Current key travel direction
  uint8_t key_dir;
  // Whether the key is pressed
  bool is_pressed;
} key_state_t;

// Key matrix
extern key_state_t key_matrix[NUM_KEYS];

//--------------------------------------------------------------------+
// Key Matrix API
//--------------------------------------------------------------------+

/**
 * @brief Initialize the key matrix module
 *
 * @return None
 */
void matrix_init(void);

/**
 * @brief Restart the calibration process
 *
 * This function will block until the calibration process is complete.
 *
 * @param reset_bottom_out_threshold Whether to reset the saved bottom-out
 * threshold as well
 *
 * @return None
 */
void matrix_recalibrate(bool reset_bottom_out_threshold);

/**
 * @brief Update the key matrix to reflect the current state of the keys
 *
 * @return None
 */
void matrix_scan(void);

/**
 * @brief Disable Rapid Trigger of a key
 *
 * This function is used by the advanced keys that override the Rapid Trigger
 * settings e.g. Dynamic Keystroke.
 *
 * @param key Key index
 * @param disable Whether to disable Rapid Trigger
 *
 * @return None
 */
void matrix_disable_rapid_trigger(uint8_t key, bool disable);
