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

#include "user_config.h"

#include <memory.h>

#include "crc32.h"
#include "eeprom.h"
#include "switches.h"

extern const user_config_t default_user_config;
user_config_t user_config;

/**
 * @brief Calculate the CRC32 of the user configuration
 *
 * @return The CRC32 of the user configuration
 */
static inline uint32_t user_config_crc32(void) {
    return crc32_bitwise(0, (const uint8_t *)&user_config + 4,
                         sizeof(user_config_t) - 4);
}

/**
 * @brief Update the CRC32 of the user configuration and save it
 *
 * @return none
 */
static inline void user_config_save_crc32(void) {
    user_config.crc32 = user_config_crc32();
    eeprom_write(offsetof(user_config_t, crc32), (uint8_t *)&user_config.crc32,
                 sizeof(user_config.crc32));
}

void user_config_init(void) {
    // TODO: Handle version migration

    // Load the user configuration from EEPROM
    eeprom_read(0, (uint8_t *)&user_config, sizeof(user_config_t));

    // Verify the user configuration
    if (user_config.crc32 != user_config_crc32())
        // The user configuration is invalid, reset it to the default
        user_config_reset();
}

void user_config_reset(void) {
    eeprom_erase();
    user_config = default_user_config;
    user_config_save_crc32();
    // Save the rest of the user configuration
    eeprom_write(4, (uint8_t *)&user_config + 4, sizeof(user_config_t) - 4);
}

void user_config_set_sw_id(uint8_t sw_id) {
    if (sw_id >= SW_COUNT)
        return;

    if (user_config.sw_id == sw_id)
        return;

    user_config.sw_id = sw_id;

    // Ensure the key configurations are within the switch's travel distance
    const uint16_t distance = sw_distance[sw_id];
    for (uint32_t i = 0; i < NUM_PROFILES; i++) {
        // Key configurations
        for (uint32_t j = 0; j < NUM_KEYS; j++) {
            key_config_t *config = &user_config.key_config[i][j];
            bool changed = false;

            switch (config->mode) {
            case KEY_MODE_NORMAL:
                if (config->nm.actuation_distance > distance) {
                    config->nm.actuation_distance = distance;
                    changed = true;
                }
                break;

            case KEY_MODE_RAPID_TRIGGER:
                if (config->rt.actuation_distance > distance) {
                    config->rt.actuation_distance = distance;
                    changed = true;
                }
                if (config->rt.rt_down_distance > distance) {
                    config->rt.rt_down_distance = distance;
                    changed = true;
                }
                if (config->rt.rt_up_distance > distance) {
                    config->rt.rt_up_distance = distance;
                    changed = true;
                }
                break;

            default:
                break;
            }

            if (changed)
                // Only save the key configuration if it was changed
                eeprom_write(KEY_CONFIG_OFFSET(i, j), (uint8_t *)config,
                             sizeof(key_config_t));
        }
        // Advanced key configurations
        for (uint32_t j = 0; j < NUM_ADVANCED_KEY_BINDINGS; j++) {
            advanced_key_config_t *config =
                &user_config.advanced_key_config[i][j];
            bool changed = false;

            switch (config->type) {
            case ADVANCED_KEY_DKS:
                if (config->dks.bottom_out_distance > distance) {
                    config->dks.bottom_out_distance = distance;
                    changed = true;
                }
                break;

            default:
                break;
            }

            if (changed)
                // Only save the advanced key configuration if it was changed
                eeprom_write(ADVANCED_KEY_CONFIG_OFFSET(i, j),
                             (uint8_t *)config, sizeof(advanced_key_config_t));
        }
    }

    user_config_save_crc32();
    eeprom_write(SW_ID_OFFSET, &user_config.sw_id, sizeof(uint8_t));
}

void user_config_set_current_profile(uint8_t profile) {
    if (profile >= NUM_PROFILES)
        return;

    if (user_config.current_profile == profile)
        return;

    user_config.current_profile = profile;
    user_config_save_crc32();
    eeprom_write(CURRENT_PROFILE_OFFSET,
                 (uint8_t *)&user_config.current_profile, sizeof(uint8_t));
}

void user_config_set_key_config(uint8_t profile, uint8_t index,
                                const key_config_t *key_config) {
    if (profile >= NUM_PROFILES || index >= NUM_KEYS)
        return;

    if (memcmp(&user_config.key_config[profile][index], key_config,
               sizeof(key_config_t)) == 0)
        return;

    user_config.key_config[profile][index] = *key_config;
    user_config_save_crc32();
    eeprom_write(KEY_CONFIG_OFFSET(profile, index),
                 (uint8_t *)&user_config.key_config[profile][index],
                 sizeof(key_config_t));
}

void user_config_set_keymap(uint8_t profile, uint8_t layer, uint8_t index,
                            uint16_t keymap) {
    if (profile >= NUM_PROFILES || layer >= NUM_LAYERS || index >= NUM_KEYS)
        return;

    if (user_config.keymap[profile][layer][index] == keymap)
        return;

    user_config.keymap[profile][layer][index] = keymap;
    user_config_save_crc32();
    eeprom_write(KEYMAP_OFFSET(profile, layer, index),
                 (uint8_t *)&user_config.keymap[profile][layer][index],
                 sizeof(uint16_t));
}

void user_config_set_advanced_key_config(uint8_t profile, uint8_t index,
                                         const advanced_key_config_t *config) {
    if (profile >= NUM_PROFILES || index >= NUM_ADVANCED_KEY_BINDINGS)
        return;

    if (memcmp(&user_config.advanced_key_config[profile][index], config,
               sizeof(advanced_key_config_t)) == 0)
        return;

    user_config.advanced_key_config[profile][index] = *config;
    user_config_save_crc32();
    eeprom_write(ADVANCED_KEY_CONFIG_OFFSET(profile, index),
                 (uint8_t *)&user_config.advanced_key_config[profile][index],
                 sizeof(advanced_key_config_t));
}