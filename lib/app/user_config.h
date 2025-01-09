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
#include "eeprom.h"

//--------------------------------------------------------------------+
// User Configuration Definitions
//--------------------------------------------------------------------+

#define USER_CONFIG_VERSION 0x0100

#if !defined(NUM_PROFILES)
// The number of profiles
#define NUM_PROFILES 4
#endif

#if !defined(NUM_LAYERS)
// The number of layers
#define NUM_LAYERS 4
#endif

#if !defined(NUM_KEYS)
#error "NUM_KEYS must be defined"
#endif

#if !defined(NUM_ADVANCED_KEY_BINDINGS)
// The number of advanced key bindings available for each profile
#define NUM_ADVANCED_KEY_BINDINGS 32
#endif

_Static_assert(NUM_PROFILES <= 16, "Invalid number of profiles");
_Static_assert(NUM_LAYERS <= 16, "Invalid number of layers");
_Static_assert(NUM_KEYS <= 256, "Invalid number of keys");
_Static_assert(NUM_ADVANCED_KEY_BINDINGS <= 256,
               "Invalid number of advanced key bindings");

//--------------------------------------------------------------------+
// User Configuration Types
//--------------------------------------------------------------------+

enum {
    KEY_MODE_NORMAL = 0,
    KEY_MODE_RAPID_TRIGGER,
};

typedef struct __attribute__((packed)) {
    // Actuation distance in 0.05mm
    uint8_t actuation_distance;
} key_mode_normal_t;

typedef struct __attribute__((packed)) {
    // Actuation distance in 0.05mm. Rapid trigger becomes active after this
    // distance.
    uint8_t actuation_distance;
    // Distance in 0.05mm to actuate the key when rapid trigger is active
    uint8_t rt_down_distance;
    // Distance in 0.05mm to release the key when rapid trigger is active
    uint8_t rt_up_distance;
    // Continuous mode. If true, rapid trigger will only be deactivated when the
    // key is released regardless of the actuation distance.
    bool continuous;
} key_mode_rapid_trigger_t;

typedef struct __attribute__((packed)) {
    uint8_t mode;
    union {
        key_mode_normal_t nm;
        key_mode_rapid_trigger_t rt;
    };
} key_config_t;

enum {
    ADVANCED_KEY_NONE = 0,
    ADVANCED_KEY_NULL_BIND,
    ADVANCED_KEY_DKS,
    ADVANCED_KEY_TAP_HOLD,
    ADVANCED_KEY_TOGGLE,
};

enum {
    // Prioritize the key that is pressed down further
    NULL_BIND_DISTANCE = 0,
    // Prioritize last pressed key
    NULL_BIND_LAST,
    // Prioritize first key
    NULL_BIND_INDEX_0,
    // Prioritize second key
    NULL_BIND_INDEX_1,
    // Deactivate both keys if both are pressed
    NULL_BIND_NEUTRAL,
};

typedef struct __attribute__((packed)) {
    // Switch index of the first key
    uint8_t index0;
    // Keycode of the first key
    uint8_t keycode0;
    // Switch index of the second key
    uint8_t index1;
    // Keycode of the second key
    uint8_t keycode1;
    // Null bind behavior
    uint8_t behavior;
    // Whether both keys are pressed if bottomed out
    bool bottomed_out;
} advanced_key_null_bind_t;

// Used for representing all possible configurations of a single key dynamic
// keystroke. Each part of the mask roughly corresponds to the length of the
// pill-shaped bar you see in Wootility.
typedef struct __attribute__((packed)) {
    uint8_t config0 : 3;
    uint8_t config1 : 2;
    uint8_t config2 : 2;
    uint8_t config3 : 1;
} dynamic_keystroke_mask_t;

_Static_assert(sizeof(dynamic_keystroke_mask_t) == 1,
               "Invalid size of dynamic keystroke mask");

typedef struct __attribute__((packed)) {
    uint8_t keycode[4];
    dynamic_keystroke_mask_t mask[4];
    // Bottom-out distance in 0.05mm
    uint8_t bottom_out_distance;
} advanced_key_dks_t;

typedef struct __attribute__((packed)) {
    // Tapping term in milliseconds
    uint16_t tapping_term;
    uint16_t tap_keycode;
    uint16_t hold_keycode;
} advanced_key_tap_hold_t;

typedef struct __attribute__((packed)) {
    // Tapping term in milliseconds
    uint16_t tapping_term;
    uint16_t keycode;
} advanced_key_toggle_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    union {
        advanced_key_null_bind_t nb;
        advanced_key_dks_t dks;
        advanced_key_tap_hold_t th;
        advanced_key_toggle_t tg;
    };
} advanced_key_config_t;

typedef struct __attribute__((packed)) {
    // For verifying the configuration
    uint32_t crc32;
    // Configuration version
    uint16_t version;

    // Current switch used
    uint8_t sw_id;
    // Current profile
    uint8_t current_profile;

    // Key configurations. Each profile is applied throughout all layers.
    key_config_t key_config[NUM_PROFILES][NUM_KEYS];
    // Keymap for each profile and layer
    uint16_t keymap[NUM_PROFILES][NUM_LAYERS][NUM_KEYS];
    // Advanced key configurations
    advanced_key_config_t advanced_key_config[NUM_PROFILES]
                                             [NUM_ADVANCED_KEY_BINDINGS];
} user_config_t;

_Static_assert(sizeof(user_config_t) <= EEPROM_BYTES,
               "User configuration size exceeds EEPROM size");

extern user_config_t user_config;

//--------------------------------------------------------------------+
// User Configuration Macros
//--------------------------------------------------------------------+

#define SW_ID_OFFSET offsetof(user_config_t, sw_id)
#define CURRENT_PROFILE_OFFSET offsetof(user_config_t, current_profile)
#define KEY_CONFIG_OFFSET(profile, index)                                      \
    (offsetof(user_config_t, key_config) +                                     \
     sizeof(key_config_t) * (profile * NUM_KEYS + index))
#define KEYMAP_OFFSET(profile, layer, index)                                   \
    (offsetof(user_config_t, keymap) +                                         \
     sizeof(uint16_t) *                                                        \
         (profile * NUM_LAYERS * NUM_KEYS + layer * NUM_KEYS + index))
#define ADVANCED_KEY_CONFIG_OFFSET(profile, index)                             \
    (offsetof(user_config_t, advanced_key_config) +                            \
     sizeof(advanced_key_config_t) *                                           \
         (profile * NUM_ADVANCED_KEY_BINDINGS + index))

//--------------------------------------------------------------------+
// User Configuration APIs
//--------------------------------------------------------------------+

/**
 * @brief Initialize the user configuration module
 *
 * @return none
 */
void user_config_init(void);

/**
 * @brief Reset the user configuration to the default configuration and save it
 *
 * @return none
 */
void user_config_reset(void);

/**
 * @brief Set the switch ID in the user configuration
 *
 * @param sw_id The switch ID
 *
 * @return none
 */
void user_config_set_sw_id(uint8_t sw_id);

/**
 * @brief Set the current profile in the user configuration
 *
 * @param profile The profile
 *
 * @return none
 */
void user_config_set_current_profile(uint8_t profile);

/**
 * @brief Set the key configuration in the user configuration
 *
 * @param profile The profile
 * @param index The key index
 * @param key_config The key configuration
 *
 * @return none
 */
void user_config_set_key_config(uint8_t profile, uint8_t index,
                                const key_config_t *key_config);

/**
 * @brief Set the keymap in the user configuration
 *
 * @param profile The profile
 * @param layer The layer
 * @param index The key index
 * @param keycode The keycode
 *
 * @return none
 */
void user_config_set_keymap(uint8_t profile, uint8_t layer, uint8_t index,
                            uint16_t keycode);

/**
 * @brief Set the advanced key configuration in the user configuration
 *
 * @param profile The profile
 * @param index The advanced key index
 * @param config The advanced key configuration
 *
 * @return none
 */
void user_config_set_advanced_key_config(uint8_t profile, uint8_t index,
                                         const advanced_key_config_t *config);