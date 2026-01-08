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
#include "wear_leveling.h"

//--------------------------------------------------------------------+
// Keyboard Persistent Configuration
//--------------------------------------------------------------------+

// Magic number to identify the start of the configuration
#define EECONFIG_MAGIC_START 0x0A42494C
// Magic number to identify the end of the configuration
#define EECONFIG_MAGIC_END 0x0A4B4D48

// Keyboard calibration configuration
typedef struct __attribute__((packed)) {
  // Initial rest value of the key matrix. If the value is smaller than the
  // actual rest value, the key will have a dead zone at the beginning of the
  // keystroke. If the value is larger than the actual rest value, a longer
  // calibration process may be required.
  uint16_t initial_rest_value;
  // Minimum change in ADC values for the key to be considered bottom-out. If
  // the value is larger than the actual bottom-out threshold, the key will have
  // a dead zone at the end of the keystroke. If the value is smaller than the
  // actual bottom-out threshold, the distance calculation may be inaccurate
  // until the first bottom-out event.
  uint16_t initial_bottom_out_threshold;
} eeconfig_calibration_t;

// Keyboard options configuration
typedef union __attribute__((packed)) {
  struct __attribute__((packed)) {
    // Whether the XInput interface is enabled
    bool xinput_enabled : 1;
    // Whether to save the bottom-out threshold
    bool save_bottom_out_threshold : 1;
    // Whether 8kHz polling rate is enabled. Only applicable if USB HS is
    // enabled. If disabled, the 1kHz polling rate is used instead.
    bool high_polling_rate_enabled : 1;
    // Reserved bits for future use
    uint16_t reserved : 13;
  };
  uint16_t raw;
} eeconfig_options_t;

_Static_assert(sizeof(eeconfig_options_t) == sizeof(uint16_t),
               "Invalid eeconfig_options_t size");

// Keyboard profile configuration
typedef struct __attribute__((packed)) {
  uint8_t keymap[NUM_LAYERS][NUM_KEYS];
  actuation_t actuation_map[NUM_KEYS];
  advanced_key_t advanced_keys[NUM_ADVANCED_KEYS];
  uint8_t gamepad_buttons[NUM_KEYS];
  gamepad_options_t gamepad_options;
  uint8_t tick_rate;
} eeconfig_profile_t;

// Persistent configuration version. The size of the configuration must be
// non-decreasing, so that the migration can assume that the new version is at
// least as large as the previous version.
#define EECONFIG_VERSION 0x0104

// Keyboard configuration
// Whenever there is a change in the configuration, `EECONFIG_VERSION` must be
// bumped. Make sure to update `eeconfig_reset()`, and add a migration function
// in `migration.c`.
typedef struct __attribute__((packed)) {
  // Global configurations
  // Magic number to identify the start of the configuration
  uint32_t magic_start;
  // Version of the configuration
  uint16_t version;

  // Calibration configuration
  eeconfig_calibration_t calibration;
  // Saved bottom-out threshold
  uint16_t bottom_out_threshold[NUM_KEYS];
  // Options configuration
  eeconfig_options_t options;

  // Current profile index
  uint8_t current_profile;
  // Last non-default profile index, used for profile swapping
  uint8_t last_non_default_profile;
  // End of global configurations

  // Profiles
  eeconfig_profile_t profiles[NUM_PROFILES];

  // Magic number to identify the end of the configuration
  uint32_t magic_end;
} eeconfig_t;

_Static_assert(
    sizeof(eeconfig_t) <= WL_VIRTUAL_SIZE,
    "Keyboard configuration size must be at most the virtual storage size.");

extern const eeconfig_t *eeconfig;

#define CURRENT_PROFILE (eeconfig->profiles[eeconfig->current_profile])

//--------------------------------------------------------------------+
// Default Keyboard Configuration
//--------------------------------------------------------------------+

#if !defined(DEFAULT_CALIBRATION)
#error "DEFAULT_CALIBRATION is not defined"
#endif

#if !defined(DEFAULT_OPTIONS)
// Default global options
#define DEFAULT_OPTIONS                                                        \
  {                                                                            \
      .xinput_enabled = false,                                                 \
      .save_bottom_out_threshold = true,                                       \
      .high_polling_rate_enabled = true,                                       \
  }
#endif

#if !defined(DEFAULT_PROFILE_KEYMAPS)
#error "DEFAULT_PROFILE_KEYMAPS is not defined"
#endif

#if !defined(DEFAULT_ACTUATION_POINT)
// Default actuation point
#define DEFAULT_ACTUATION_POINT 128
#endif

#if !defined(DEFAULT_GAMEPAD_OPTIONS)
// Default gamepad options
#define DEFAULT_GAMEPAD_OPTIONS                                                \
  {                                                                            \
      .analog_curve = {{4, 20}, {85, 95}, {165, 170}, {255, 255}},             \
      .keyboard_enabled = true,                                                \
      .snappy_joystick = true,                                                 \
  }
#endif

#if !defined(DEFAULT_TICK_RATE)
// Default tick rate
#define DEFAULT_TICK_RATE 30
#endif

//--------------------------------------------------------------------+
// Persistent Configuration API
//--------------------------------------------------------------------+

/**
 * @brief Initialize the persistent configuration module
 *
 * @return None
 */
void eeconfig_init(void);

/**
 * @brief Reset the persistent configuration to default values
 *
 * @return true if successful, false otherwise
 */
bool eeconfig_reset(void);

/**
 * @brief Reset a specific profile to default values
 *
 * @param profile Profile index
 *
 * @return true if successful, false otherwise
 */
bool eeconfig_reset_profile(uint8_t profile);

/**
 * @brief Write a value to a field in the persistent configuration
 *
 * @param field Field to write to
 * @param value Value to write
 *
 * @return true if successful, false otherwise
 */
#define EECONFIG_WRITE(field, value)                                           \
  wear_leveling_write(offsetof(eeconfig_t, field), value,                      \
                      sizeof(((eeconfig_t *)0)->field))

/**
 * @brief Write a value to a field in the persistent configuration
 *
 * @param field Field to write to
 * @param value Value to write
 * @param len Length of the value in bytes
 *
 * @return true if successful, false otherwise
 */
#define EECONFIG_WRITE_N(field, value, len)                                    \
  wear_leveling_write(offsetof(eeconfig_t, field), value, len)
