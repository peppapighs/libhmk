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

/* clang-format off
   ___   __   _  _  ____  ____   ___   ___ 
  / __) / _\ / )( \/ ___)/ ___) / __) / _ \
 ( (_ \/    \) \/ (\___ \\___ \(  _ \(__  (
  \___/\_/\_/\____/(____/(____/ \___/  (__/

clang-format on */

#include "adc.h"
#include "keycodes.h"
#include "main.h"
#include "switches.h"
#include "user_config.h"

// CD74HC4067
#define NUM_MUX 4

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim10;

static const uint16_t select_pins[] = {
    MUX_SELECT_0_Pin,
    MUX_SELECT_1_Pin,
    MUX_SELECT_2_Pin,
    MUX_SELECT_3_Pin,
};

static GPIO_TypeDef *select_ports[] = {
    MUX_SELECT_0_GPIO_Port,
    MUX_SELECT_1_GPIO_Port,
    MUX_SELECT_2_GPIO_Port,
    MUX_SELECT_3_GPIO_Port,
};

static const uint16_t matrix[][NUM_MUX] = {
    {63, 51, 47, 57}, {54, 37, 33, 58}, {55, 52, 48, 44}, {40, 38, 34, 59},
    {41, 24, 20, 2},  {13, 10, 6, 1},   {27, 9, 5, 16},   {26, 23, 19, 30},
    {12, 8, 4, 15},   {11, 22, 18, 29}, {25, 7, 3, 0},    {60, 21, 17, 14},
    {61, 35, 31, 28}, {62, 49, 45, 56}, {53, 50, 46, 42}, {39, 36, 32, 43},
};

// ADC DMA buffer
static uint16_t adc_buffer[NUM_MUX];

//--------------------------------------------------------------------+
// User Implementations
//--------------------------------------------------------------------+

void adc_init(void) {
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, NUM_MUX);
}

uint16_t calibration_initial_min(uint8_t sw_id) { return 2400; }

uint16_t calibration_delta(uint8_t sw_id) { return 650; }

//--------------------------------------------------------------------+
// HAL Callbacks
//--------------------------------------------------------------------+

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    static uint8_t mux_index = 0;

    if (hadc == &hadc1) {
        for (uint32_t i = 0; i < NUM_MUX; i++)
            // Invert the ADC value since we use voltage followers
            store_adc_value(matrix[mux_index][i], 4095 - adc_buffer[i]);

        mux_index = (mux_index + 1) & 0xF;
        for (uint32_t i = 0; i < 4; i++)
            HAL_GPIO_WritePin(select_ports[i], select_pins[i],
                              (GPIO_PinState)((mux_index >> i) & 1));

        // Delay before the next ADC conversion to allow the voltage to settle
        HAL_TIM_Base_Start_IT(&htim10);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim10) {
        HAL_TIM_Base_Stop_IT(&htim10);
        HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, NUM_MUX);
    }
}

//--------------------------------------------------------------------+
// Default User Configuration
//--------------------------------------------------------------------+

#define NM                                                                     \
    {                                                                          \
        .mode = KEY_MODE_NORMAL,                                               \
        .nm =                                                                  \
            {                                                                  \
                .actuation_distance = 40,                                      \
            },                                                                 \
    }
#define RT                                                                     \
    {                                                                          \
        .mode = KEY_MODE_RAPID_TRIGGER,                                        \
        .rt =                                                                  \
            {                                                                  \
                .actuation_distance = 2,                                       \
                .rt_down_distance = 2,                                         \
                .rt_up_distance = 2,                                           \
                .continuous = false,                                           \
            },                                                                 \
    }
#define DIGITAL_KEY_CONFIG                                                     \
    {                                                                          \
        NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM,        \
        NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM,        \
        NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM,        \
        NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM,        \
    }
#define ANALOG_KEY_CONFIG                                                      \
    {                                                                          \
        NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM,        \
        RT, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, RT, RT, RT,        \
        NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM, NM,        \
        NM, NM, NM, NM, NM, NM, RT, NM, NM, NM, NM, RT, NM, RT, RT, RT,        \
    }

#define LT_VOLD AK(0)
#define MGC_BTL SP_MAGIC_BOOTLOADER
#define MGC_RBT SP_MAGIC_REBOOT
#define MGC_FAC SP_MAGIC_FACTORY_RESET
#define MGC_REC SP_MAGIC_RECALIBRATE
#define DEFAULT_KEYMAP                                                         \
    {                                                                          \
        {                                                                      \
            KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,        \
            KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,     \
            KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,        \
            KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,     \
            KC_ESC,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,        \
            KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_ENT,  KC_VOLU,     \
            KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,        \
            KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, KC_UP,   LT_VOLD,     \
            KC_LCTL, KC_LGUI, KC_LALT, KC_SPC,  KC_RALT, KC_LEFT, KC_DOWN,     \
            KC_RGHT,                                                           \
        },                                                                     \
        {                                                                      \
            _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,       \
            KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  _______,     \
            _______, PTO(0),  PTO(1),  PTO(2),  PTO(3),  _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, MGC_BTL, MGC_RBT, MGC_FAC, MGC_REC, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______,                                                           \
        },                                                                     \
        {                                                                      \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______,                                                           \
        },                                                                     \
        {                                                                      \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______, _______, _______, _______, _______, _______, _______,     \
            _______,                                                           \
        },                                                                     \
    }

#define DEFAULT_ADVANCED_KEY_CONFIG                                            \
    {                                                                          \
        [0] = {                                                                \
            .type = ADVANCED_KEY_TAP_HOLD,                                     \
            .th =                                                              \
                {                                                              \
                    .tap_keycode = KC_VOLD,                                    \
                    .hold_keycode = MO(1),                                     \
                    .tapping_term = 200,                                       \
                },                                                             \
        }                                                                      \
    }

const user_config_t default_user_config = {
    .crc32 = 0,
    .version = USER_CONFIG_VERSION,

    .sw_id = SW_GATERON_MAGNETIC_JADE,
    .current_profile = 0,

    .key_config =
        {
            DIGITAL_KEY_CONFIG,
            ANALOG_KEY_CONFIG,
            ANALOG_KEY_CONFIG,
            ANALOG_KEY_CONFIG,
        },
    .keymap =
        {
            DEFAULT_KEYMAP,
            DEFAULT_KEYMAP,
            DEFAULT_KEYMAP,
            DEFAULT_KEYMAP,
        },
    .advanced_key_config =
        {
            DEFAULT_ADVANCED_KEY_CONFIG,
            DEFAULT_ADVANCED_KEY_CONFIG,
            DEFAULT_ADVANCED_KEY_CONFIG,
            DEFAULT_ADVANCED_KEY_CONFIG,
        },
};
