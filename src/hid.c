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

#include "bitmap.h"
#include "commands.h"
#include "keycodes.h"
#include "matrix.h"
#include "tusb.h"
#include "usb_descriptors.h"

// Track how many keys are currently in the 6KRO part of the report
static uint8_t num_6kro_keys;
static hid_nkro_kb_report_t kb_report;

static uint16_t system_report;
static uint16_t consumer_report;
static hid_mouse_report_t mouse_report;

#if !defined(HID_DISABLED)
/**
 * @brief Send the keyboard report
 *
 * This function will send the keyboard report to its exclusive interface.
 *
 * @return None
 */
static void hid_send_keyboard_report(void) {
  static hid_nkro_kb_report_t prev_kb_report = {0};

  if (memcmp(&prev_kb_report, &kb_report, sizeof(prev_kb_report)) == 0)
    // Don't send the report if it hasn't changed
    return;

  prev_kb_report = kb_report;
  if (tud_hid_n_report(USB_ITF_KEYBOARD, 0, &kb_report, sizeof(kb_report)))
    matrix_reset_inactivity_timer();
}
#endif

/**
 * @brief Find the next available report and send it
 *
 * @param starting_report_id The report ID to start searching from
 *
 * @return None
 */
static void hid_send_hid_report(uint8_t starting_report_id) {
  static uint16_t prev_system_report = 0;
  static uint16_t prev_consumer_report = 0;
  static hid_mouse_report_t prev_mouse_report = {0};

  for (uint8_t report_id = starting_report_id; report_id < REPORT_ID_COUNT;
       report_id++) {
    switch (report_id) {
    case REPORT_ID_SYSTEM_CONTROL:
      if (system_report == prev_system_report)
        // Don't send the report if it hasn't changed
        break;
      prev_system_report = system_report;
      if (tud_hid_n_report(USB_ITF_HID, report_id, &system_report,
                           sizeof(system_report)))
        matrix_reset_inactivity_timer();
      return;

    case REPORT_ID_CONSUMER_CONTROL:
      if (consumer_report == prev_consumer_report)
        // Don't send the report if it hasn't changed
        break;
      prev_consumer_report = consumer_report;
      if (tud_hid_n_report(USB_ITF_HID, report_id, &consumer_report,
                           sizeof(consumer_report)))
        matrix_reset_inactivity_timer();
      return;

    case REPORT_ID_MOUSE:
      if (memcmp(&prev_mouse_report, &mouse_report,
                 sizeof(prev_mouse_report)) == 0)
        // Don't send the report if it hasn't changed
        break;
      prev_mouse_report = mouse_report;
      if (tud_hid_n_report(USB_ITF_HID, report_id, &mouse_report,
                           sizeof(mouse_report)))
        matrix_reset_inactivity_timer();
      return;

    default:
      break;
    }
  }
}

void hid_init(void) {}

void hid_keycode_add(uint8_t keycode) {
  const uint16_t hid_code = keycode_to_hid[keycode];

  if (!hid_code)
    // No HID code for this keycode
    return;

  bool found = false;
  switch (keycode) {
  case KEYBOARD_KEYCODE_RANGE:
    for (uint32_t i = 0; i < num_6kro_keys; i++) {
      if (kb_report.keycodes[i] == hid_code) {
        found = true;
        break;
      }
    }

    if (!found) {
      if (num_6kro_keys == 6) {
        // If the 6KRO report is full, remove the oldest key
        for (uint32_t i = 0; i < 5; i++)
          kb_report.keycodes[i] = kb_report.keycodes[i + 1];
        num_6kro_keys--;
      }
      kb_report.keycodes[num_6kro_keys++] = hid_code;
    }
    kb_report.bitmap[hid_code / 8] |= 1 << (hid_code & 7);
    break;

  case MODIFIER_KEYCODE_RANGE:
    kb_report.modifiers |= hid_code;
    break;

  case SYSTEM_KEYCODE_RANGE:
    system_report = hid_code;
    break;

  case CONSUMER_KEYCODE_RANGE:
    consumer_report = hid_code;
    break;

  case MOUSE_KEYCODE_RANGE:
    mouse_report.buttons |= hid_code;
    break;

  default:
    break;
  }
}

void hid_keycode_remove(uint8_t keycode) {
  const uint16_t hid_code = keycode_to_hid[keycode];

  if (!hid_code)
    // No HID code for this keycode
    return;

  switch (keycode) {
  case KEYBOARD_KEYCODE_RANGE:
    for (uint32_t i = 0; i < num_6kro_keys; i++) {
      if (kb_report.keycodes[i] == hid_code) {
        for (uint32_t j = i; j < 5; j++)
          kb_report.keycodes[j] = kb_report.keycodes[j + 1];
        num_6kro_keys--;
        break;
      }
    }
    kb_report.bitmap[hid_code / 8] &= ~(1 << (hid_code & 7));
    break;

  case MODIFIER_KEYCODE_RANGE:
    kb_report.modifiers &= ~hid_code;
    break;

  case SYSTEM_KEYCODE_RANGE:
    if (system_report == hid_code)
      // Only remove the system report if it matches the one we're trying to
      system_report = 0;
    break;

  case CONSUMER_KEYCODE_RANGE:
    if (consumer_report == hid_code)
      // Only remove the consumer report if it matches the one we're trying to
      consumer_report = 0;
    break;

  case MOUSE_KEYCODE_RANGE:
    mouse_report.buttons &= ~hid_code;
    break;

  default:
    break;
  }
}

void hid_send_reports(void) {
#if !defined(HID_DISABLED)
  if (tud_suspended())
    // Wake up the host if it's suspended
    tud_remote_wakeup();

  while (!tud_hid_n_ready(USB_ITF_KEYBOARD))
    // Wait for the keyboard interface to be ready
    tud_task();

  hid_send_keyboard_report();

  while (!tud_hid_n_ready(USB_ITF_HID))
    // Wait for the HID interface to be ready
    tud_task();

  // Start from the first report ID
  hid_send_hid_report(REPORT_ID_SYSTEM_CONTROL);
#endif
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, const uint8_t *buffer,
                           uint16_t bufsize) {
  if (instance == USB_ITF_RAW_HID)
    command_process(buffer);
}

void tud_hid_report_complete_cb(uint8_t instance, const uint8_t *report,
                                uint16_t len) {
  if (instance == USB_ITF_HID)
    // Start from the next report ID
    hid_send_hid_report(report[0] + 1);
}
