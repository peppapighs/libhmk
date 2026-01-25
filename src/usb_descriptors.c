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

#include "usb_descriptors.h"

#include "eeconfig.h"
#include "hardware/hardware.h"
#include "metadata.h"
#include "tusb.h"
#include "xinput.h"

// Device descriptor
static const tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD_VERSION,

    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = USB_VENDOR_ID,
    .idProduct = USB_PRODUCT_ID,
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01,
};

// HID report descriptor for keyboard interface
static const uint8_t desc_keyboard_report[] = {
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
    HID_USAGE(HID_USAGE_DESKTOP_KEYBOARD),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),

    // 8 bits for modifiers
    HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD), HID_USAGE_MIN(224),
    HID_USAGE_MAX(231), HID_LOGICAL_MIN(0), HID_LOGICAL_MAX(1),
    HID_REPORT_COUNT(8), HID_REPORT_SIZE(1),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

    // 8 bits reserved
    HID_REPORT_COUNT(1), HID_REPORT_SIZE(8), HID_INPUT(HID_CONSTANT),

    // 5-bit LED indicator output
    HID_USAGE_PAGE(HID_USAGE_PAGE_LED), HID_USAGE_MIN(1), HID_USAGE_MAX(5),
    HID_REPORT_COUNT(5), HID_REPORT_SIZE(1),
    HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

    // LED padding
    HID_REPORT_COUNT(1), HID_REPORT_SIZE(3), HID_OUTPUT(HID_CONSTANT),

    // 6-byte padding for compatibility with 6-KRO HID report
    HID_REPORT_COUNT(48), HID_REPORT_SIZE(1), HID_INPUT(HID_CONSTANT),

    // NKRO bitmap
    HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD), HID_USAGE_MIN(0),
    HID_USAGE_MAX(NUM_NKRO_BYTES * 8 - 1), HID_LOGICAL_MIN(0),
    HID_LOGICAL_MAX(1), HID_REPORT_COUNT(NUM_NKRO_BYTES * 8),
    HID_REPORT_SIZE(1), HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_COLLECTION_END

};

// HID report descriptor for other HID interfaces
static const uint8_t desc_hid_report[] = {
    TUD_HID_REPORT_DESC_SYSTEM_CONTROL(HID_REPORT_ID(REPORT_ID_SYSTEM_CONTROL)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
};

// HID report descriptor for the raw HID interface
static const uint8_t desc_raw_hid_report[] = {
    HID_USAGE_PAGE_N(RAW_HID_USAGE_PAGE, 2), HID_USAGE(RAW_HID_USAGE),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),

    // Data to host
    HID_USAGE(RAW_HID_USAGE + 1), HID_LOGICAL_MIN(0), HID_LOGICAL_MAX_N(255, 2),
    HID_REPORT_COUNT(RAW_HID_EP_SIZE), HID_REPORT_SIZE(8),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

    // Data from host
    HID_USAGE(RAW_HID_USAGE + 2), HID_LOGICAL_MIN(0), HID_LOGICAL_MAX_N(255, 2),
    HID_REPORT_COUNT(RAW_HID_EP_SIZE), HID_REPORT_SIZE(8),
    HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_NON_VOLATILE),
    HID_COLLECTION_END

};

#define CONFIG_TOTAL_LEN                                                       \
  (TUD_CONFIG_DESC_LEN + 2 * TUD_HID_DESC_LEN + TUD_HID_INOUT_DESC_LEN +       \
   XINPUT_DESC_LEN)

// Configuration descriptor
static uint8_t desc_configuration[CONFIG_TOTAL_LEN];

#if defined(BOARD_USB_HS)
// Device qualifier descriptor for USB HS
static const tusb_desc_device_qualifier_t desc_device_qualifier = {
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = USB_BCD_VERSION,

    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0x00,
};

// Other speed configuration descriptor for USB HS. Same as the main
// configuration descriptor
static uint8_t desc_other_speed_config[CONFIG_TOTAL_LEN];
#endif

// String descriptor
static const char *desc_strings[] = {
    // Supported languages (English)
    [STR_ID_LANGID] = (const char[]){U16_TO_U8S_LE(0x0409)},
    // Manufacturer
    [STR_ID_MANUFACTURER] = USB_MANUFACTURER_NAME,
    // Product
    [STR_ID_PRODUCT] = USB_PRODUCT_NAME,
    // Serial number (will be filled by the callback)
    [STR_ID_SERIAL] = NULL,
};

_Static_assert(M_ARRAY_SIZE(desc_strings) == STR_ID_COUNT,
               "Invalid string descriptor size");

#define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

// BOS descriptor
static const uint8_t desc_bos[] = {
    // Total length of the BOS descriptor
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    // Microsoft OS 2.0 descriptor
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, MS_OS_20_VENDOR_CODE),
};

// Microsoft OS 2.0 descriptor
static const uint8_t desc_ms_os_20[] = {
    // Set header length
    U16_TO_U8S_LE(0x000A),
    // Set header descriptor type
    U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    // Minimum supported Windows version (Windows 8.1)
    U32_TO_U8S_LE(0x06030000),
    // Total length of MS OS 2.0 descriptor
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

    // Subset header length
    U16_TO_U8S_LE(0x0008),
    // Subset header descriptor type
    U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
    // Configuration index
    0x00,
    // Reserved
    0x00,
    // Total length of subset including header
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x000A),

    // Function subset header length
    U16_TO_U8S_LE(0x0008),
    // Function subset header descriptor type
    U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
    // First interface
    USB_ITF_XINPUT,
    // Reserved
    0x00,
    // Total length of function subset including header
    U16_TO_U8S_LE(0x00A0),

    // Feature descriptor length
    U16_TO_U8S_LE(0x0014),
    // Feature descriptor type
    U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    // Compatible ID
    'X', 'U', 'S', 'B', '2', '0', 0x00, 0x00,
    // Sub-compatible ID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Registry property descriptor length
    U16_TO_U8S_LE(0x0084),
    // Registry property descriptor type
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    // Property data type (REG_MULTI_SZ)
    U16_TO_U8S_LE(0x0007),
    // Property name length
    U16_TO_U8S_LE(0x002A),
    // Property name
    U16_TO_U8S_LE('D'), U16_TO_U8S_LE('e'), U16_TO_U8S_LE('v'),
    U16_TO_U8S_LE('i'), U16_TO_U8S_LE('c'), U16_TO_U8S_LE('e'),
    U16_TO_U8S_LE('I'), U16_TO_U8S_LE('n'), U16_TO_U8S_LE('t'),
    U16_TO_U8S_LE('e'), U16_TO_U8S_LE('r'), U16_TO_U8S_LE('f'),
    U16_TO_U8S_LE('a'), U16_TO_U8S_LE('c'), U16_TO_U8S_LE('e'),
    U16_TO_U8S_LE('G'), U16_TO_U8S_LE('U'), U16_TO_U8S_LE('I'),
    U16_TO_U8S_LE('D'), U16_TO_U8S_LE('s'), U16_TO_U8S_LE('\0'),
    // Property data length
    U16_TO_U8S_LE(0x0050),
    // Property data (random GUID)
    MS_OS_20_GUID

};

_Static_assert(M_ARRAY_SIZE(desc_ms_os_20) == MS_OS_20_DESC_LEN,
               "Invalid Microsoft OS 2.0 descriptor size");

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

/**
 * @brief Generate the configuration descriptor based on the current persistent
 * configuration
 *
 * @param dst Buffer to write to
 *
 * @return None
 */
static void generate_desc_configuration(uint8_t *dst) {
  uint8_t num_interfaces = USB_ITF_COUNT;
  uint16_t total_length = CONFIG_TOTAL_LEN;
  if (!eeconfig->options.xinput_enabled) {
    // If XInput is not enabled, subtract the XInput descriptor length
    // from the total configuration length.
    num_interfaces--;
    total_length -= XINPUT_DESC_LEN;
  }

  uint8_t polling_interval = 1;
#if defined(BOARD_USB_HS)
  if (!eeconfig->options.high_polling_rate_enabled)
    // If high polling rate is not enabled, use 1kHz polling rate = 8 frames for
    // USB HS instead.
    polling_interval = 8;
#endif

  const uint8_t src[] = {
      // Configuration descriptor header. Request maximum 500mA for the device
      TUD_CONFIG_DESCRIPTOR(1, num_interfaces, 0, total_length,
                            TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),
      // Keyboard interface descriptor
      TUD_HID_DESCRIPTOR(USB_ITF_KEYBOARD, 0, HID_ITF_PROTOCOL_KEYBOARD,
                         sizeof(desc_keyboard_report), EP_IN_ADDR_KEYBOARD,
                         CFG_TUD_HID_EP_BUFSIZE, polling_interval),
      // HID interface descriptor
      TUD_HID_DESCRIPTOR(USB_ITF_HID, 0, HID_ITF_PROTOCOL_NONE,
                         sizeof(desc_hid_report), EP_IN_ADDR_HID,
                         CFG_TUD_HID_EP_BUFSIZE, polling_interval),
      // Raw HID interface descriptor
      TUD_HID_INOUT_DESCRIPTOR(USB_ITF_RAW_HID, 0, HID_ITF_PROTOCOL_NONE,
                               sizeof(desc_raw_hid_report), EP_OUT_ADDR_RAW_HID,
                               EP_IN_ADDR_RAW_HID, RAW_HID_EP_SIZE,
                               polling_interval),
      // XInput interface descriptor
      XINPUT_DESCRIPTOR(USB_ITF_XINPUT, 0, EP_OUT_ADDR_XINPUT,
                        EP_IN_ADDR_XINPUT, polling_interval),
  };

  _Static_assert(sizeof(src) == CONFIG_TOTAL_LEN,
                 "Invalid configuration descriptor size");

  memcpy(dst, src, sizeof(src));
}

const uint8_t *tud_descriptor_device_cb(void) {
  return (const uint8_t *)&desc_device;
}

const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) {
  switch (instance) {
  case USB_ITF_KEYBOARD:
    return desc_keyboard_report;

  case USB_ITF_HID:
    return desc_hid_report;

  case USB_ITF_RAW_HID:
    return desc_raw_hid_report;

  default:
    // Invalid interface, should be unreachable
    return NULL;
  }
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
  // We only have one configuration so we don't need to check the index
  generate_desc_configuration(desc_configuration);
  return desc_configuration;
}

#if defined(BOARD_USB_HS)
const uint8_t *tud_descriptor_device_qualifier_cb(void) {
  return (const uint8_t *)&desc_device_qualifier;
}

const uint8_t *tud_descriptor_other_speed_configuration_cb(uint8_t index) {
  generate_desc_configuration(desc_other_speed_config);
  desc_other_speed_config[1] = TUSB_DESC_OTHER_SPEED_CONFIG;

  return desc_other_speed_config;
}
#endif

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  static uint16_t desc_str[32 + 1];

  uint32_t chr_count;
  switch (index) {
  case STR_ID_LANGID: {
    memcpy(desc_str + 1, desc_strings[STR_ID_LANGID], 2);
    chr_count = 1;
    break;
  }
  case STR_ID_SERIAL: {
    char serial_buf[32];
    chr_count = board_serial(serial_buf);
    // Convert ASCII to UTF-16-LE
    for (uint32_t i = 0; i < chr_count; i++)
      desc_str[i + 1] = serial_buf[i];
    break;
  }
  default: {
    if (index >= STR_ID_COUNT)
      // Unknown string. Note that index 0xEE is reserved for Microsoft OS 1.0
      // descriptor, but we don't use it.
      return NULL;

    const char *str = desc_strings[index];

    // Truncate string if it is too long
    const uint32_t max_len = M_ARRAY_SIZE(desc_str) - 1;
    chr_count = strlen(str);
    if (chr_count > max_len)
      chr_count = max_len;

    // Convert ASCII string into UTF-16-LE
    for (uint32_t i = 0; i < chr_count; i++)
      desc_str[i + 1] = str[i];
    break;
  }
  }

  // First byte is the length of the descriptor, second byte is the type
  desc_str[0] = (2 * chr_count + 2) | ((uint16_t)TUSB_DESC_STRING << 8);

  return desc_str;
}

const uint8_t *tud_descriptor_bos_cb(void) { return desc_bos; }

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                const tusb_control_request_t *request) {
  if (stage != CONTROL_STAGE_SETUP)
    // Nothing to do for other stages
    return true;

  if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
      request->bRequest == MS_OS_20_VENDOR_CODE) {
    switch (request->wIndex) {
    case 0x07:
      // Microsoft OS 2.0 descriptor request
      return tud_control_xfer(rhport, request, (void *)desc_ms_os_20,
                              MS_OS_20_DESC_LEN);

    default:
      // Unsupported request
      break;
    }
  }

  return false;
}
