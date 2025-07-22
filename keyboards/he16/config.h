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
// Clock Configuration
//--------------------------------------------------------------------+

#define BOARD_HSE_VALUE 12000000

//--------------------------------------------------------------------+
// USB Configuration
//--------------------------------------------------------------------+

#define BOARD_USB_HS

#define USB_MANUFACTURER_NAME "ABS0"
#define USB_PRODUCT_NAME "HE16"
#define USB_VENDOR_ID 0xAB50
#define USB_PRODUCT_ID 0xAB16

//--------------------------------------------------------------------+
// Analog Configuration
//--------------------------------------------------------------------+

#define ADC_NUM_MUX_INPUTS 2
#define ADC_MUX_INPUT_CHANNELS {1, 0}

// SN74LV4051A 8-Channel Analog Multiplexer
#define ADC_NUM_MUX_SELECT_PINS 3
#define ADC_MUX_SELECT_PORTS {GPIOC, GPIOC, GPIOC}
#define ADC_MUX_SELECT_PINS {GPIO_PINS_13, GPIO_PINS_14, GPIO_PINS_15}
#define ADC_MUX_INPUT_MATRIX                                                   \
  {                                                                            \
      {2, 10}, {4, 11}, {3, 12}, {1, 9}, {6, 16}, {5, 13}, {7, 15}, {8, 14},   \
  }

//--------------------------------------------------------------------+
// Keyboard Configuration
//--------------------------------------------------------------------+

// Approximated by measuring the actual values of GEON Raw HE switches
#define DEFAULT_CALIBRATION                                                    \
  {                                                                            \
      .initial_rest_value = 2400,                                              \
      .initial_bottom_out_threshold = 650,                                     \
  }

#define NUM_KEYS 16

#define DEFAULT_KEYMAP                                                         \
  {                                                                            \
      [0 ... NUM_LAYERS - 1] =                                                 \
          {                                                                    \
              [0 ... NUM_KEYS - 1] = _______,                                  \
          },                                                                   \
  }

#define DEFAULT_ACTUATION_MAP                                                  \
  {                                                                            \
      [0 ... NUM_KEYS - 1] =                                                   \
          {                                                                    \
              .actuation_point = 128,                                          \
              .rt_down = 0,                                                    \
              .rt_up = 0,                                                      \
              .continuous = false,                                             \
          },                                                                   \
  }

#define DEFAULT_ADVANCED_KEYS                                                  \
  {                                                                            \
  }

#define DEFAULT_PROFILE                                                        \
  {                                                                            \
      .keymap = DEFAULT_KEYMAP,                                                \
      .actuation_map = DEFAULT_ACTUATION_MAP,                                  \
      .advanced_keys = DEFAULT_ADVANCED_KEYS,                                  \
      .tick_rate = 30,                                                         \
  }
