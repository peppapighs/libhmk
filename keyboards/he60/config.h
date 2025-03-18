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

#define BOARD_HSE_VALUE 16000000

//--------------------------------------------------------------------+
// USB Configuration
//--------------------------------------------------------------------+

#define BOARD_USB_FS

#define USB_MANUFACTURER_NAME "ABS0"
#define USB_PRODUCT_NAME "HE60"
#define USB_VENDOR_ID 0xAB50
#define USB_PRODUCT_ID 0xAB60

//--------------------------------------------------------------------+
// Analog Configuration
//--------------------------------------------------------------------+

#define ADC_NUM_MUX_INPUTS 9
#define ADC_MUX_INPUT_CHANNELS {3, 4, 5, 6, 7, 14, 15, 9, 8}

// TMUX1308 8-Channel Analog Multiplexer
#define ADC_NUM_MUX_SELECT_PINS 3
#define ADC_MUX_SELECT_PORTS {GPIOC, GPIOC, GPIOC}
#define ADC_MUX_SELECT_PINS {GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15}
#define ADC_MUX_INPUT_MATRIX                                                    \
  {                                                                             \
      {3, 15, 21, 29, 36, 56, 59, 66, 62}, {2, 14, 0, 28, 35, 43, 58, 67, 63},  \
      {1, 13, 0, 27, 0, 42, 57, 54, 64},   {4, 16, 22, 30, 37, 55, 60, 65, 61}, \
      {5, 9, 17, 23, 32, 38, 31, 50, 47},  {8, 12, 20, 26, 0, 41, 46, 53, 49},  \
      {6, 10, 18, 24, 33, 39, 44, 51, 0},  {7, 11, 19, 25, 34, 40, 45, 52, 48}, \
  }

//--------------------------------------------------------------------+
// Key Matrix Configuration
//--------------------------------------------------------------------+

// The raw ADC values are not directly proportional to the travel distance of
// the keys so we must invert the values.
#define MATRIX_INVERT_ADC_VALUES

// Approximated by measuring the actual values of GEON Raw HE switches
#define MATRIX_INITIAL_REST_VALUE 2400
#define MATRIX_INITIAL_BOTTOM_OUT_THRESHOLD 550

//--------------------------------------------------------------------+
// Keyboard Configuration
//--------------------------------------------------------------------+

#define NUM_KEYS 67

#define DEFAULT_KEYMAP                                                         \
  {                                                                            \
      {                                                                        \
          KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,          \
          KC_7,    KC_8,    KC_9,    KC_0,    KC_EQL,  KC_MINS, KC_BSLS,       \
          _______, KC_DEL,  KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,          \
          KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC,       \
          KC_RBRC, KC_BSPC, KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,          \
          KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,       \
          KC_ENT,  KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,          \
          KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, MO(1),         \
          KC_LCTL, KC_LGUI, KC_LALT, _______, _______, KC_SPC,  _______,       \
          _______, KC_RALT, KC_RGUI, KC_RCTL,                                  \
      },                                                                       \
      {                                                                        \
          KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,         \
          KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  _______,       \
          _______, KC_INS,  KC_PSCR, _______, KC_PGUP, _______, _______,       \
          _______, _______, _______, _______, PF(0),   PF(1),   PF(2),         \
          PF(3),   _______, _______, KC_HOME, KC_PGDN, KC_END,  _______,       \
          _______, _______, KC_MPRV, KC_MPLY, KC_MNXT, _______, _______,       \
          PF_SWAP, _______, _______, _______, _______, _______, _______,       \
          _______, KC_MUTE, KC_VOLD, KC_VOLU, KC_UP,   _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, KC_LEFT, KC_DOWN, KC_RGHT,                                  \
      },                                                                       \
      {                                                                        \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______,                                  \
      },                                                                       \
      {                                                                        \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______, _______, _______, _______,       \
          _______, _______, _______, _______,                                  \
      },                                                                       \
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
  }
