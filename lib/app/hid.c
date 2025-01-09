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

#include "hid.h"

#include "keycodes.h"
#include "post_hid_report.h"
#include "tusb.h"
#include "usb_descriptors.h"

static uint8_t num_6kro_keycodes;
static hid_hybrid_kb_report_t kb_report;
static uint16_t system_report;
static uint16_t consumer_report;

void hid_init(void) {
    num_6kro_keycodes = 0;
    memset(&kb_report, 0, sizeof(kb_report));
    system_report = 0;
    consumer_report = 0;
}

/**
 * @brief Add a keycode to the HID report
 *
 * @param keycode The HID keycode to add (not the firmware keycode)
 *
 * @return none
 */
static void hid_add_hid_keycode(uint8_t keycode) {
    if (!keycode)
        return;

    bool found = false;
    for (uint32_t i = 0; i < num_6kro_keycodes; i++) {
        if (kb_report.keycodes[i] == keycode) {
            found = true;
            break;
        }
    }

    if (!found) {
        if (num_6kro_keycodes == 6) {
            // If the 6-KRO buffer is full, remove the oldest key.
            for (uint32_t i = 0; i < 5; i++)
                kb_report.keycodes[i] = kb_report.keycodes[i + 1];
            num_6kro_keycodes--;
        }
        kb_report.keycodes[num_6kro_keycodes++] = keycode;
    }

    // Update the bitmap
    kb_report.bitmap[keycode >> 3] |= 1 << (keycode & 0x07);
}

/**
 * @brief Remove a keycode from the HID report
 *
 * @param keycode The HID keycode to remove (not the firmware keycode)
 *
 * @return none
 */
static void hid_remove_hid_keycode(uint8_t keycode) {
    if (!keycode)
        return;

    for (uint32_t i = 0; i < num_6kro_keycodes; i++) {
        if (kb_report.keycodes[i] == keycode) {
            // Remove the key and shift the rest of the keys
            for (uint32_t j = i; j < num_6kro_keycodes - 1; j++)
                kb_report.keycodes[j] = kb_report.keycodes[j + 1];
            kb_report.keycodes[num_6kro_keycodes--] = 0;
            break;
        }
    }

    // Update the bitmap
    kb_report.bitmap[keycode >> 3] &= ~(1 << (keycode & 0x07));
}

void hid_add_modifier(uint8_t modifier) { kb_report.modifiers |= modifier; }

void hid_remove_modifier(uint8_t modifier) { kb_report.modifiers &= ~modifier; }

void hid_add_keycode(uint16_t keycode) {
    if (!IS_HID_KEYCODE(keycode))
        return;

    if (IS_BASIC_KEYCODE(keycode))
        hid_add_hid_keycode(keycode);
    else if (IS_SYSTEM_KEYCODE(keycode))
        system_report = keycode_to_system(keycode);
    else if (IS_CONSUMER_KEYCODE(keycode))
        consumer_report = keycode_to_consumer(keycode);
    else if (IS_MODIFIER_KEYCODE(keycode))
        hid_add_modifier(keycode_to_modifier(keycode));
}

void hid_remove_keycode(uint16_t keycode) {
    if (!IS_HID_KEYCODE(keycode))
        return;

    if (IS_BASIC_KEYCODE(keycode))
        hid_remove_hid_keycode(keycode);
    else if (IS_SYSTEM_KEYCODE(keycode))
        system_report = 0;
    else if (IS_CONSUMER_KEYCODE(keycode))
        consumer_report = 0;
    else if (IS_MODIFIER_KEYCODE(keycode))
        hid_remove_modifier(keycode_to_modifier(keycode));
}

/**
 * @brief Find the next report to send and send it
 *
 * @param starting_report_id The report ID to start from
 *
 * @return true if the report was sent successfully, false otherwise
 */
static bool hid_send_report(uint8_t starting_report_id) {
    static hid_hybrid_kb_report_t prev_kb_report = {0};
    static uint16_t prev_system_report = 0;
    static uint16_t prev_consumer_report = 0;

    for (uint8_t report_id = starting_report_id; report_id < REPORT_ID_COUNT;
         report_id++) {
        switch (report_id) {
        case REPORT_ID_KEYBOARD:
            if (memcmp(&kb_report, &prev_kb_report, sizeof(kb_report)) == 0)
                // The report has not changed
                break;

            prev_kb_report = kb_report;
            tud_hid_report(REPORT_ID_KEYBOARD, &kb_report, sizeof(kb_report));
            return true;

        case REPORT_ID_SYSTEM_CONTROL:
            if (system_report == prev_system_report)
                // The report has not changed
                break;

            prev_system_report = system_report;
            tud_hid_report(REPORT_ID_SYSTEM_CONTROL, &system_report,
                           sizeof(system_report));
            return true;

        case REPORT_ID_CONSUMER_CONTROL:
            if (consumer_report == prev_consumer_report)
                // The report has not changed
                break;

            prev_consumer_report = consumer_report;
            tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &consumer_report,
                           sizeof(consumer_report));
            return true;

        default:
            break;
        }
    }

    // If no report was sent, it means we have no more reports to send and we
    // can run the post-HID-report task.
    post_hid_report_task();
    return false;
}

void hid_send_reports(void) {
    if (tud_suspended())
        // Wake up host if we are in suspend mode
        tud_remote_wakeup();

    while (!tud_hid_ready())
        tud_task();

    hid_send_report(REPORT_ID_KEYBOARD);
}

//--------------------------------------------------------------------+
// TinyUSB HID Callbacks
//--------------------------------------------------------------------+

/**
 * @brief HID GET_REPORT callback
 *
 * Invoked when received GET_REPORT control request.
 *
 * @param instance      HID instance
 * @param report_id     Report ID
 * @param report_type   Report type
 * @param buffer        Buffer to store report data
 * @param reqlen        Requested length
 *
 * @return number of bytes written to buffer or zero to STALL request
 */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    return 0;
}

/**
 * @brief HID SET_REPORT callback
 *
 * Invoked when received SET_REPORT control request or received data on OUT
 * endpoint (Report ID = 0, Type = 0)
 *
 * @param instance      HID instance
 * @param report_id     Report ID
 * @param report_type   Report type
 * @param buffer        Report data
 * @param bufsize       Report data length
 *
 * @return none
 */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, const uint8_t *buffer,
                           uint16_t bufsize) {}

/**
 * @brief HID REPORT sent callback
 *
 * Invoked when sent REPORT successfully to host. Handle sending next pending
 * report.
 *
 * @param instance      HID instance
 * @param report        Report data
 * @param len           Report data length
 *
 * @return none
 */
void tud_hid_report_complete_cb(uint8_t instance, const uint8_t *report,
                                uint16_t len) {
    // Try to send the next report
    hid_send_report(report[0] + 1);
}