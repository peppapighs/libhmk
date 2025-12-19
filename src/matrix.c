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

#include "matrix.h"

#include "bitmap.h"
#include "distance.h"
#include "eeconfig.h"
#include "hardware/hardware.h"

// Exponential moving average (EMA) filter
#define EMA(x, y)                                                              \
  (((uint32_t)(x) +                                                            \
    ((uint32_t)(y) * ((1 << MATRIX_EMA_ALPHA_EXPONENT) - 1))) >>               \
   MATRIX_EMA_ALPHA_EXPONENT)

__attribute__((always_inline)) static inline uint16_t
matrix_analog_read(uint8_t key) {
#if defined(MATRIX_INVERT_ADC_VALUES)
  return ADC_MAX_VALUE - analog_read(key);
#else
  return analog_read(key);
#endif
}

__attribute__((always_inline)) static inline uint16_t
matrix_bottom_out_value(uint8_t key, uint16_t rest_value) {
  return M_MIN(rest_value +
                   M_MAX(eeconfig->calibration.initial_bottom_out_threshold,
                         eeconfig->bottom_out_threshold[key]),
               ADC_MAX_VALUE);
}

key_state_t key_matrix[NUM_KEYS];

// Bitmap for tracking which keys have Rapid Trigger disabled
static bitmap_t rapid_trigger_disabled[] = MAKE_BITMAP(NUM_KEYS);
// Last time the bottom-out threshold changed
static uint32_t last_bottom_out_threshold_changed;

/**
 * @brief Save the current bottom-out threshold to the persistent configuration
 *
 * @return None
 */
static void matrix_save_bottom_out_threshold(void) {
  uint16_t bottom_out_threshold[NUM_KEYS];

  for (uint32_t i = 0; i < NUM_KEYS; i++) {
    if (key_matrix[i].adc_bottom_out_value >= key_matrix[i].adc_rest_value)
      bottom_out_threshold[i] =
          key_matrix[i].adc_bottom_out_value - key_matrix[i].adc_rest_value;
    else
      bottom_out_threshold[i] = 0;
  }
  EECONFIG_WRITE(bottom_out_threshold, bottom_out_threshold);
  // Reset the timer to delay the next save
  last_bottom_out_threshold_changed = timer_read();
}

void matrix_init(void) { matrix_recalibrate(false); }

void matrix_recalibrate(bool reset_bottom_out_threshold) {
  if (reset_bottom_out_threshold) {
    uint16_t bottom_out_threshold[NUM_KEYS] = {0};
    EECONFIG_WRITE(bottom_out_threshold, bottom_out_threshold);
  }

  for (uint32_t i = 0; i < NUM_KEYS; i++) {
    key_matrix[i].adc_filtered = eeconfig->calibration.initial_rest_value;
    key_matrix[i].adc_rest_value = eeconfig->calibration.initial_rest_value;
    key_matrix[i].adc_bottom_out_value =
        matrix_bottom_out_value(i, eeconfig->calibration.initial_rest_value);
    key_matrix[i].distance = 0;
    key_matrix[i].extremum = 0;
    key_matrix[i].key_dir = KEY_DIR_INACTIVE;
    key_matrix[i].is_pressed = false;
  }

  // We only calibrate the rest value. The bottom-out value will be updated
  // during the scan process.
  const uint32_t calibration_start = timer_read();
  while (timer_elapsed(calibration_start) < MATRIX_CALIBRATION_DURATION) {
    // Run the analog task to possibly update the ADC values
    analog_task();

    for (uint32_t i = 0; i < NUM_KEYS; i++) {
      const uint16_t new_adc_filtered =
          EMA(matrix_analog_read(i), key_matrix[i].adc_filtered);

      key_matrix[i].adc_filtered = new_adc_filtered;

      if (new_adc_filtered + MATRIX_CALIBRATION_EPSILON <=
          key_matrix[i].adc_rest_value)
        // Only update the rest value if the new value is smaller and the
        // difference is at least the calibration epsilon
        key_matrix[i].adc_rest_value = new_adc_filtered;

      // Update the bottom-out value to be the minimum bottom-out value based on
      // the updated rest value
      key_matrix[i].adc_bottom_out_value =
          matrix_bottom_out_value(i, key_matrix[i].adc_rest_value);
    }
  }
  // Reset the timer after calibration
  last_bottom_out_threshold_changed = timer_read();
}

void matrix_scan(void) {
  for (uint32_t i = 0; i < NUM_KEYS; i++) {
    const uint16_t new_adc_filtered =
        EMA(matrix_analog_read(i), key_matrix[i].adc_filtered);
    const actuation_t *actuation = &CURRENT_PROFILE.actuation_map[i];

    key_matrix[i].adc_filtered = new_adc_filtered;

    if (new_adc_filtered >=
        key_matrix[i].adc_bottom_out_value + MATRIX_CALIBRATION_EPSILON) {
      // Only update the bottom-out value if the new value is larger and the
      // difference is at least the calibration epsilon.
      key_matrix[i].adc_bottom_out_value = new_adc_filtered;
      last_bottom_out_threshold_changed = timer_read();
    }

    key_matrix[i].distance =
        adc_to_distance(new_adc_filtered, key_matrix[i].adc_rest_value,
                        key_matrix[i].adc_bottom_out_value);

    if (bitmap_get(rapid_trigger_disabled, i) | (actuation->rt_down == 0)) {
      key_matrix[i].key_dir = KEY_DIR_INACTIVE;
      key_matrix[i].is_pressed =
          (key_matrix[i].distance >= actuation->actuation_point);
    } else {
      const uint8_t reset_point =
          actuation->continuous ? 0 : actuation->actuation_point;
      const uint8_t rt_up =
          actuation->rt_up == 0 ? actuation->rt_down : actuation->rt_up;

      switch (key_matrix[i].key_dir) {
      case KEY_DIR_INACTIVE:
        if (key_matrix[i].distance > actuation->actuation_point) {
          // Pressed down past actuation point
          key_matrix[i].extremum = key_matrix[i].distance;
          key_matrix[i].key_dir = KEY_DIR_DOWN;
          key_matrix[i].is_pressed = true;
        }
        break;

      case KEY_DIR_DOWN:
        if (key_matrix[i].distance <= reset_point) {
          // Released past reset point
          key_matrix[i].extremum = key_matrix[i].distance;
          key_matrix[i].key_dir = KEY_DIR_INACTIVE;
          key_matrix[i].is_pressed = false;
        } else if (key_matrix[i].distance + rt_up < key_matrix[i].extremum) {
          // Released by Rapid Trigger
          key_matrix[i].extremum = key_matrix[i].distance;
          key_matrix[i].key_dir = KEY_DIR_UP;
          key_matrix[i].is_pressed = false;
        } else if (key_matrix[i].distance > key_matrix[i].extremum)
          // Pressed down further
          key_matrix[i].extremum = key_matrix[i].distance;
        break;

      case KEY_DIR_UP:
        if (key_matrix[i].distance <= reset_point) {
          // Released past reset point
          key_matrix[i].extremum = key_matrix[i].distance;
          key_matrix[i].key_dir = KEY_DIR_INACTIVE;
          key_matrix[i].is_pressed = false;
        } else if (key_matrix[i].extremum + actuation->rt_down <
                   key_matrix[i].distance) {
          // Pressed by Rapid Trigger
          key_matrix[i].extremum = key_matrix[i].distance;
          key_matrix[i].key_dir = KEY_DIR_DOWN;
          key_matrix[i].is_pressed = true;
        } else if (key_matrix[i].distance < key_matrix[i].extremum)
          // Released further
          key_matrix[i].extremum = key_matrix[i].distance;
        break;

      default:
        break;
      }
    }
  }

  // Save the bottom-out threshold after inactivity if enabled
  if (eeconfig->options.save_bottom_out_threshold &&
      timer_elapsed(last_bottom_out_threshold_changed) >=
          MATRIX_INACTIVITY_TIMEOUT)
    matrix_save_bottom_out_threshold();
}

void matrix_disable_rapid_trigger(uint8_t key, bool disable) {
  bitmap_set(rapid_trigger_disabled, key, disable);
}
