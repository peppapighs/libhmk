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

#include "vendor.h"

#include "app.h"
#include "board.h"
#include "switches.h"
#include "tusb.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// TinyUSB Vendor Callbacks
//--------------------------------------------------------------------+

// Not the best way to access the user configuration, but it saves us from
// defining temporary buffers for each response.
extern const user_config_t user_config;

#if defined(ENABLE_WEB_CONFIGURATOR)
static uint8_t request_buffer[VENDOR_REQUEST_BUFFER_SIZE];
#endif

#if defined(USB_WEBUSB_URL)
// WebUSB landing page URL
const tusb_desc_webusb_url_t desc_url = {
    .bLength = 3 + sizeof(USB_WEBUSB_URL) - 1, // Total length
    .bDescriptorType = TUSB_DESC_STRING,       // WebUSB URL type
    .bScheme = USB_WEBUSB_URL_SCHEME,          // URL scheme
    .url = USB_WEBUSB_URL,                     // WebUSB URL
};
#endif

/**
 * @brief Vendor control transfer callback
 *
 * Invoked when a control transfer occurred on an interface of this class.
 * Handle Microsoft OS 2.0 descriptor request. If `USB_WEBUSB_URL` is defined,
 * handle WebUSB request as well.
 *
 * @param rhport Root hub port number
 * @param stage Transfer stage
 * (`CONTROL_STAGE_SETUP`/`CONTROL_STAGE_DATA`/`CONTROL_STAGE_ACK`)
 * @param request Control request
 *
 * @return true if the request is supported and can be processed.
 */
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                tusb_control_request_t const *request) {
    switch (request->bmRequestType_bit.type) {
    case TUSB_REQ_TYPE_VENDOR:
        switch (request->bRequest) {
#if defined(USB_WEBUSB_URL)
        case VENDOR_REQUEST_WEBUSB:
            if (stage == CONTROL_STAGE_SETUP)
                // Get landing page URL
                return tud_control_xfer(rhport, request, (void *)&desc_url,
                                        desc_url.bLength);
            // Nothing to do for DATA & ACK stages
            return true;
#endif

        case VENDOR_REQUEST_MICROSOFT:
            if (stage == CONTROL_STAGE_SETUP && request->wIndex == 0x07) {
                // Get Microsoft OS 2.0 compatible descriptor
                uint16_t total_len;
                memcpy(&total_len, desc_ms_os_20 + 8, 2);

                return tud_control_xfer(rhport, request, (void *)desc_ms_os_20,
                                        total_len);
            }
            // Nothing to do for DATA & ACK stages
            return true;

        default:
            break;
        }

        break;

#if defined(ENABLE_WEB_CONFIGURATOR)
    case TUSB_REQ_TYPE_CLASS:
        switch (request->bRequest) {
        case CLASS_REQUEST_PROTOCOL_VERSION:
            if (stage == CONTROL_STAGE_SETUP) {
                static const uint16_t res = VENDOR_CLASS_PROTOCOL_VERSION;

                if (request->wLength < sizeof(res))
                    // Invalid response length
                    return false;

                return tud_control_xfer(rhport, request, (void *)&res,
                                        sizeof(res));
            }
            // Nothing to do for DATA & ACK stages
            return true;

        case CLASS_REQUEST_FIRMWARE_VERSION:
            if (stage == CONTROL_STAGE_SETUP) {
                static const uint16_t res = FIRMWARE_VERSION;

                if (request->wLength < sizeof(res))
                    // Invalid response length
                    return false;

                return tud_control_xfer(rhport, request, (void *)&res,
                                        sizeof(res));
            }
            // Nothing to do for DATA & ACK stages
            return true;

        case CLASS_REQUEST_BOOTLOADER:
#if defined(ENABLE_BOOTLOADER)
            if (stage == CONTROL_STAGE_SETUP) {
                board_enter_bootloader();
                return tud_control_status(rhport, request);
            }
#endif
            // Nothing to do for DATA & ACK stages
            return true;

        case CLASS_REQUEST_REBOOT:
            if (stage == CONTROL_STAGE_SETUP) {
                board_reset();
                return tud_control_status(rhport, request);
            }
            // Nothing to do for DATA & ACK stages
            return true;

        case CLASS_REQUEST_FACTORY_RESET:
            if (stage == CONTROL_STAGE_SETUP) {
                user_config_reset();
                return tud_control_status(rhport, request);
            }
            // Nothing to do for DATA & ACK stages
            return true;

        case CLASS_REQUEST_RECALIBRATE:
            if (stage == CONTROL_STAGE_SETUP) {
                switch_recalibrate();
                return tud_control_status(rhport, request);
            }
            // Nothing to do for DATA & ACK stages
            return true;

        case CLASS_REQUEST_SWITCH_DEBUG:
            if (stage == CONTROL_STAGE_SETUP) {
                static class_res_switch_debug_t res;

                if (request->wLength < sizeof(res))
                    // Invalid response length
                    return false;

                for (uint32_t i = 0; i < NUM_KEYS; i++) {
                    res.adc_values[i] = get_switch_adc_value(i);
                    res.distances[i] = get_switch_distance(i);
                }

                return tud_control_xfer(rhport, request, (void *)&res,
                                        sizeof(res));
            }
            // Nothing to do for DATA & ACK stages
            return true;

        case CLASS_REQUEST_SW_ID:
            switch (request->wValue) {
            case CLASS_REQUEST_INDEX_GET:
                if (stage == CONTROL_STAGE_SETUP) {
                    const uint32_t len = sizeof(user_config.sw_id);

                    if (request->wLength < len)
                        // Invalid response length
                        return false;

                    return tud_control_xfer(rhport, request,
                                            (void *)&user_config.sw_id, len);
                }
                // Nothing to do for DATA & ACK stages
                return true;

            case CLASS_REQUEST_INDEX_SET:
                if (stage == CONTROL_STAGE_SETUP) {
                    if (request->wLength != sizeof(user_config.sw_id))
                        // Invalid request length
                        return false;

                    return tud_control_xfer(rhport, request, request_buffer,
                                            request->wLength);
                } else if (stage == CONTROL_STAGE_DATA) {
                    user_config_set_sw_id(request_buffer[0]);
                    return true;
                }
                // Nothing to do for ACK stage
                return true;

            default:
                break;
            }
            break;

        case CLASS_REQUEST_KEY_CONFIG:
            switch (request->wValue) {
            case CLASS_REQUEST_INDEX_GET:
                if (stage == CONTROL_STAGE_SETUP) {
                    const uint32_t len = sizeof(user_config.key_config);

                    if (request->wLength < len)
                        // Invalid response length
                        return false;

                    return tud_control_xfer(
                        rhport, request, (void *)user_config.key_config, len);
                }
                // Nothing to do for DATA & ACK stages
                return true;

            case CLASS_REQUEST_INDEX_SET:
                if (stage == CONTROL_STAGE_SETUP) {
                    if (request->wLength % sizeof(class_req_key_config_t) !=
                            0 ||
                        request->wLength > VENDOR_REQUEST_BUFFER_SIZE)
                        // Invalid request length
                        return false;

                    return tud_control_xfer(rhport, request, request_buffer,
                                            request->wLength);
                } else if (stage == CONTROL_STAGE_DATA) {
                    const uint32_t num_reqs =
                        request->wLength / sizeof(class_req_key_config_t);
                    const class_req_key_config_t *reqs =
                        (class_req_key_config_t *)request_buffer;

                    for (uint32_t i = 0; i < num_reqs; i++)
                        user_config_set_key_config(reqs[i].profile,
                                                   reqs[i].index,
                                                   &reqs[i].key_config);

                    return true;
                }
                // Nothing to do for ACK stage
                return true;

            default:
                break;
            }
            break;

        case CLASS_REQUEST_KEYMAP:
            switch (request->wValue) {
            case CLASS_REQUEST_INDEX_GET:
                if (stage == CONTROL_STAGE_SETUP) {
                    const uint32_t len = sizeof(user_config.keymap);

                    if (request->wLength < len)
                        // Invalid response length
                        return false;

                    return tud_control_xfer(rhport, request,
                                            (void *)user_config.keymap, len);
                }
                // Nothing to do for DATA & ACK stages
                return true;

            case CLASS_REQUEST_INDEX_SET:
                if (stage == CONTROL_STAGE_SETUP) {
                    if (request->wLength % sizeof(class_req_keymap_t) != 0 ||
                        request->wLength > VENDOR_REQUEST_BUFFER_SIZE)
                        // Invalid request length
                        return false;

                    return tud_control_xfer(rhport, request, request_buffer,
                                            request->wLength);
                } else if (stage == CONTROL_STAGE_DATA) {
                    const uint32_t num_reqs =
                        request->wLength / sizeof(class_req_keymap_t);
                    const class_req_keymap_t *reqs =
                        (class_req_keymap_t *)request_buffer;

                    for (uint32_t i = 0; i < num_reqs; i++)
                        user_config_set_keymap(reqs[i].profile, reqs[i].layer,
                                               reqs[i].index, reqs[i].keycode);

                    return true;
                }
                // Nothing to do for ACK stage
                return true;

            default:
                break;
            }
            break;

        case CLASS_REQUEST_ADVANCED_KEY_CONFIG:
            switch (request->wValue) {
            case CLASS_REQUEST_INDEX_GET:
                if (stage == CONTROL_STAGE_SETUP) {
                    const uint32_t len =
                        sizeof(user_config.advanced_key_config);

                    if (request->wLength < len)
                        // Invalid response length
                        return false;

                    return tud_control_xfer(
                        rhport, request,
                        (void *)user_config.advanced_key_config, len);
                }
                // Nothing to do for DATA & ACK stages
                return true;

            case CLASS_REQUEST_INDEX_SET:
                if (stage == CONTROL_STAGE_SETUP) {
                    if (request->wLength %
                                sizeof(class_req_advanced_key_config_t) !=
                            0 ||
                        request->wLength > VENDOR_REQUEST_BUFFER_SIZE)
                        // Invalid request length
                        return false;

                    return tud_control_xfer(rhport, request, request_buffer,
                                            request->wLength);
                } else if (stage == CONTROL_STAGE_DATA) {
                    const uint32_t num_reqs =
                        request->wLength /
                        sizeof(class_req_advanced_key_config_t);
                    const class_req_advanced_key_config_t *reqs =
                        (class_req_advanced_key_config_t *)request_buffer;

                    for (uint32_t i = 0; i < num_reqs; i++)
                        user_config_set_advanced_key_config(
                            reqs[i].profile, reqs[i].index,
                            &reqs[i].advanced_key_config);

                    return true;
                }
                // Nothing to do for ACK stage
                return true;

            default:
                break;
            }
            break;

        default:
            break;
        }
        break;
#endif

    default:
        break;
    }

    // Stall unknown request
    return false;
}
