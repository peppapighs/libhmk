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

#include "device/usbd_pvt.h"
#include "log.h"
#include "tusb.h"
#include "usb_descriptors.h"

static uint8_t endpoint_in;
static uint8_t endpoint_out;

bool xinput_send_report(const xinput_report_t *report) {
  bool success = false;

  if (tud_ready() && endpoint_in != 0 && !usbd_edpt_busy(0, endpoint_in)) {
    usbd_edpt_claim(0, endpoint_in);
    success = usbd_edpt_xfer(0, endpoint_in, (uint8_t *)report,
                             sizeof(xinput_report_t));
    usbd_edpt_release(0, endpoint_in);
  }

  return success;
}

//---------------------------------------------------------------------+
// TinyUSB Callbacks
//---------------------------------------------------------------------+

static void xinput_init(void) {}

static void xinput_reset(uint8_t rhport) {}

static uint16_t xinput_open(uint8_t rhport,
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

static bool xinput_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                   tusb_control_request_t const *request) {
  return true;
}

static bool xinput_xfer_cb(uint8_t rhport, uint8_t ep_addr,
                           xfer_result_t result, uint32_t xferred_bytes) {
  return true;
}

static const usbd_class_driver_t xinput_driver = {
    .init = xinput_init,
    .reset = xinput_reset,
    .open = xinput_open,
    .control_xfer_cb = xinput_control_xfer_cb,
    .xfer_cb = xinput_xfer_cb,
    .sof = NULL,
};

const usbd_class_driver_t *usbd_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = 1;

  return &xinput_driver;
}
