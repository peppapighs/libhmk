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

#include "xinput.h"

#include "bitmap.h"
#include "device/usbd_pvt.h"
#include "eeconfig.h"
#include "lib/usqrt.h"
#include "matrix.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "log.h"

/**
 * @brief Convert square joystick coordinates to circular coordinates
 *
 * @param x X coordinate
 * @param y Y coordinate
 *
 * @return X in circular coordinates
 */
static uint8_t square_to_circular(uint8_t x, uint8_t y) {
  return (uint16_t)x * usqrt16(255 * 255 - (((uint16_t)y * y) >> 1)) / 255;
}

/**
 * @brief Apply the analog curve to the analog value
 *
 * We assume that the X coordinates are strictly increasing.
 *
 * @param value Analog value
 * @param[out] is_key_end_deadzone Whether the analog value is in the key end
 * deadzone
 *
 * @return Processed analog value
 */
static uint8_t apply_analog_curve(uint8_t value, bool *is_key_end_deadzone) {
  const uint8_t (*curve)[2] = CURRENT_PROFILE.gamepad_options.analog_curve;

  *is_key_end_deadzone = (value > curve[3][0]);
  if (*is_key_end_deadzone)
    // Key end deadzone
    return 255;

  if (value <= curve[0][0])
    // Key start deadzone
    return 0;

  // Find the segment in the curve where the value falls
  uint8_t i = 0;
  for (; i < 3; i++) {
    if (curve[i + 1][0] >= value)
      break;
  }

  const int16_t x1 = curve[i][0], y1 = curve[i][1];
  const int16_t x2 = curve[i + 1][0], y2 = curve[i + 1][1];

  return y1 + (y2 - y1) * (value - x1) / (x2 - x1);
}

// Mapping for digital gamepad buttons to XInput button bitmasks
static const uint16_t keycode_to_bm[] = {
    [GP_BUTTON_A] = XINPUT_BUTTON_A,
    [GP_BUTTON_B] = XINPUT_BUTTON_B,
    [GP_BUTTON_X] = XINPUT_BUTTON_X,
    [GP_BUTTON_Y] = XINPUT_BUTTON_Y,
    [GP_BUTTON_UP] = XINPUT_BUTTON_UP,
    [GP_BUTTON_DOWN] = XINPUT_BUTTON_DOWN,
    [GP_BUTTON_LEFT] = XINPUT_BUTTON_LEFT,
    [GP_BUTTON_RIGHT] = XINPUT_BUTTON_RIGHT,
    [GP_BUTTON_START] = XINPUT_BUTTON_START,
    [GP_BUTTON_BACK] = XINPUT_BUTTON_BACK,
    [GP_BUTTON_HOME] = XINPUT_BUTTON_HOME,
    [GP_BUTTON_LS] = XINPUT_BUTTON_LS,
    [GP_BUTTON_RS] = XINPUT_BUTTON_RS,
    [GP_BUTTON_LB] = XINPUT_BUTTON_LB,
    [GP_BUTTON_RB] = XINPUT_BUTTON_RB,
};

// Negative and positive axes for joysticks
static const uint8_t joystick_axes[][2] = {
    {GP_BUTTON_LS_LEFT, GP_BUTTON_LS_RIGHT}, // lx
    {GP_BUTTON_LS_DOWN, GP_BUTTON_LS_UP},    // ly
    {GP_BUTTON_RS_LEFT, GP_BUTTON_RS_RIGHT}, // rx
    {GP_BUTTON_RS_DOWN, GP_BUTTON_RS_UP},    // ry
};

// Endpoints for XInput communication
static uint8_t endpoint_in;
static uint8_t endpoint_out;

// We track key press states independently of the layout module in case
// layout processing is disabled for some keys.
static bitmap_t key_press_states[] = MAKE_BITMAP(NUM_KEYS);
// Track maximum analog values for analog buttons
// (2 joysticks * 4 directions + 2 triggers)
static uint16_t analog_states[10];

// Access analog states using the button index
#define ANALOG_STATE(button) analog_states[(button) - GP_BUTTON_LS_UP]

static xinput_report_t report = {.report_size = sizeof(xinput_report_t)};

void xinput_init(void) {}

void xinput_process(uint8_t key) {
  const key_state_t *k = &key_matrix[key];
  const uint8_t keycode = CURRENT_PROFILE.gamepad_buttons[key];

  if (keycode == GP_BUTTON_NONE)
    return;

  switch (keycode) {
  case GP_BUTTON_A ... GP_BUTTON_RB: {
    const bool last_key_press = bitmap_get(key_press_states, key);

    if (k->is_pressed & !last_key_press)
      // Key press event
      report.buttons |= keycode_to_bm[keycode];
    else if (!k->is_pressed & last_key_press)
      // Key release event
      report.buttons &= ~keycode_to_bm[keycode];

    // Finally, update the key state
    bitmap_set(key_press_states, key, k->is_pressed);
    break;
  }
  case GP_BUTTON_LS_UP ... GP_BUTTON_RT: {
    // Update the maximum analog value for the analog button
    ANALOG_STATE(keycode) = M_MAX(ANALOG_STATE(keycode), k->distance);
    break;
  }
  default: {
    break;
  }
  }
}

void xinput_task(void) {
  static xinput_report_t last_report = {.report_size = sizeof(xinput_report_t)};

  bool is_key_end_deadzone = false;
  // Update trigger states in the report
  report.lz =
      apply_analog_curve(ANALOG_STATE(GP_BUTTON_LT), &is_key_end_deadzone);
  report.rz =
      apply_analog_curve(ANALOG_STATE(GP_BUTTON_RT), &is_key_end_deadzone);

  // lx, ly, rx, ry
  uint16_t joystick_states[4] = {0};

  // Combine joystick axes based on the configuration
  for (uint32_t i = 0; i < 4; i++) {
    const uint8_t neg_axis = joystick_axes[i][0];
    const uint8_t pos_axis = joystick_axes[i][1];

    if (CURRENT_PROFILE.gamepad_options.snappy_joystick)
      // For snappy joystick, we use the maximum value of opposite axes.
      joystick_states[i] =
          M_MAX(ANALOG_STATE(neg_axis), ANALOG_STATE(pos_axis));
    else
      // Otherwise, we combine the opposite axes.
      joystick_states[i] =
          abs((int16_t)ANALOG_STATE(pos_axis) - ANALOG_STATE(neg_axis));
  }

  // Apply the analog curve to joystick states
  for (uint32_t i = 0; i < 2; i++) {
    uint16_t *state = &joystick_states[i * 2];

    uint32_t x = state[0], y = state[1];
    const uint32_t magnitude = usqrt32(x * x + y * y);
    if (magnitude == 0)
      // If magnitude is zero, skip analog curve processing
      continue;

    // Calculate the maximum magnitude for the joystick vector
    const uint32_t max_x = x > y ? 255 : x * 255 / y;
    const uint32_t max_y = y > x ? 255 : y * 255 / x;
    const uint32_t max_magnitude = usqrt32(max_x * max_x + max_y * max_y);
    // Apply the analog curve to the joystick magnitude. The magnitude is
    // scaled to [0, 255] range.
    const uint32_t new_magnitude = apply_analog_curve(
        magnitude * 255 / max_magnitude, &is_key_end_deadzone);

    if (is_key_end_deadzone) {
      // If the joystick is in the key end deadzone, we snap the axes to
      // maximum analog value.
      x = x == 0 ? 0 : 255;
      y = y == 0 ? 0 : 255;
    } else {
      // Otherwise, scale the joystick states to the new magnitude
      // We scale the maximum vector instead of the joystick vector to
      // prevent the analog values from exceeding the maximum range due to
      // approximation errors.
      x = max_x * new_magnitude / 255;
      y = max_y * new_magnitude / 255;
    }

    if (!CURRENT_PROFILE.gamepad_options.square_joystick) {
      // Convert square joystick coordinates to circular coordinates
      state[0] = square_to_circular(x, y);
      state[1] = square_to_circular(y, x);
    } else {
      // Otherwise, use the original values
      state[0] = x;
      state[1] = y;
    }
  }

  // Update joystick states in the report
  for (uint32_t i = 0; i < 4; i++) {
    const uint8_t neg_axis = joystick_axes[i][0];
    const uint8_t pos_axis = joystick_axes[i][1];
    // Scale range from [0, 255] to [0, 32767]
    const int16_t joystick_state = joystick_states[i] << 7;

    // Assign signed joystick values to the report
    if (ANALOG_STATE(pos_axis) > ANALOG_STATE(neg_axis))
      // Positive axis
      report.joysticks[i] = joystick_state;
    else
      // Negative axis
      report.joysticks[i] = -joystick_state;
  }

  if (tud_ready() && endpoint_in != 0 && !usbd_edpt_busy(0, endpoint_in) &&
      // Only send report if it has changed
      memcmp(&report, &last_report, sizeof(xinput_report_t)) != 0) {
    usbd_edpt_claim(0, endpoint_in);
    usbd_edpt_xfer(0, endpoint_in, (uint8_t *)&report, sizeof(xinput_report_t));
    usbd_edpt_release(0, endpoint_in);
    // Update the last report to the current one
    memcpy(&last_report, &report, sizeof(xinput_report_t));
  }

  // Reset analog states for the next scan
  memset(analog_states, 0, sizeof(analog_states));
}

//---------------------------------------------------------------------+
// TinyUSB Callbacks
//---------------------------------------------------------------------+

static void xinput_driver_init(void) {}

static void xinput_driver_reset(uint8_t rhport) {}

static uint16_t xinput_driver_open(uint8_t rhport,
                                   const tusb_desc_interface_t *desc_intf,
                                   uint16_t max_len) {
  if (desc_intf->bInterfaceClass == TUSB_CLASS_VENDOR_SPECIFIC &&
      desc_intf->bInterfaceSubClass == 0x5D &&
      desc_intf->bInterfaceProtocol == 0x01) {
    TU_ASSERT(usbd_open_edpt_pair(rhport, tu_desc_next(tu_desc_next(desc_intf)),
                                  desc_intf->bNumEndpoints, TUSB_XFER_INTERRUPT,
                                  &endpoint_out, &endpoint_in),
              0);

    return XINPUT_DESC_LEN;
  }

  return 0;
}

static bool
xinput_driver_control_xfer_cb(uint8_t rhport, uint8_t stage,
                              tusb_control_request_t const *request) {
  return true;
}

static bool xinput_driver_xfer_cb(uint8_t rhport, uint8_t ep_addr,
                                  xfer_result_t result,
                                  uint32_t xferred_bytes) {
  return true;
}

static const usbd_class_driver_t xinput_driver = {
    .init = xinput_driver_init,
    .reset = xinput_driver_reset,
    .open = xinput_driver_open,
    .control_xfer_cb = xinput_driver_control_xfer_cb,
    .xfer_cb = xinput_driver_xfer_cb,
    .sof = NULL,
};

const usbd_class_driver_t *usbd_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = 1;

  return &xinput_driver;
}
