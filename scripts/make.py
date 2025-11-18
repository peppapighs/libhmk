# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

import utils

Import("env")

keyboard = env["PIOENV"]

# Load JSON files. We assume that they have been validated in `get_deps.py`.
kb_json = utils.get_kb_json(keyboard)
driver_json = utils.get_driver_json(keyboard)
driver = kb_json["hardware"]["driver"]

# Add source filter for driver source files
env.Append(SRC_FILTER=["-<hardware/>", f"+<hardware/{driver}/>"])

# Build Flags
build_flags = utils.CompilerFlags()

# Include headers. We prioritize including driver and keyboard headers.
build_flags.include(f"hardware/{driver}")
build_flags.include(f"keyboards/{keyboard}")
build_flags.include("include")

# TinyUSB Configuration
build_flags.define("CFG_TUSB_MCU", f"OPT_MCU_{driver_json['tinyusb']['mcu'].upper()}")

# Clock Configuration
build_flags.define("BOARD_HSE_VALUE", kb_json["hardware"]["hse_value"])
build_flags.define("HSE_VALUE", kb_json["hardware"]["hse_value"])

# USB Configuration
if kb_json["usb"]["port"] == "fs":
    build_flags.define("BOARD_USB_FS")
else:
    build_flags.define("BOARD_USB_HS")
build_flags.define("USB_MANUFACTURER_NAME", f"\"{kb_json['manufacturer']}\"")
build_flags.define("USB_PRODUCT_NAME", f"\"{kb_json['name']}\"")
build_flags.define("USB_VENDOR_ID", kb_json["usb"]["vid"])
build_flags.define("USB_PRODUCT_ID", kb_json["usb"]["pid"])

# Analog Configuration
build_flags.define("ADC_RESOLUTION", utils.get_adc_resolution(kb_json, driver_json))

if kb_json["analog"].get("invert_adc", False):
    build_flags.define("MATRIX_INVERT_ADC_VALUES")

if "delay" in kb_json["analog"]:
    build_flags.define("ADC_SAMPLE_DELAY", kb_json["analog"]["delay"])

# Raw ADC Input Configuration
if "raw" in kb_json["analog"]:
    raw = kb_json["analog"]["raw"]

    build_flags.define("ADC_NUM_RAW_INPUTS", len(raw["input"]))
    build_flags.define(
        "ADC_RAW_INPUT_CHANNELS", utils.to_adc_input_array(raw["input"], driver_json)
    )
    build_flags.define("ADC_RAW_INPUT_VECTOR", utils.to_c_array(raw["vector"]))

# Analog Multiplexer ADC Input Configuration
if "mux" in kb_json["analog"]:
    mux = kb_json["analog"]["mux"]

    build_flags.define("ADC_NUM_MUX_INPUTS", len(mux["input"]))
    build_flags.define(
        "ADC_MUX_INPUT_CHANNELS", utils.to_adc_input_array(mux["input"], driver_json)
    )
    build_flags.define("ADC_NUM_MUX_SELECT_PINS", len(mux["select"]))

    ports, pin_nums = utils.to_gpio_array(mux["select"], driver)
    build_flags.define("ADC_MUX_SELECT_PORTS", utils.to_c_array(ports))
    build_flags.define("ADC_MUX_SELECT_PINS", utils.to_c_array(pin_nums))

    build_flags.define(
        "ADC_MUX_INPUT_MATRIX", utils.to_c_array(list(map(list, zip(*mux["matrix"]))))
    )

# Calibration Configuration
build_flags.define("DEFAULT_CALIBRATION", utils.to_c_struct(kb_json["calibration"]))

# Keyboard Configuration
kb = kb_json["keyboard"]
build_flags.define("NUM_PROFILES", kb["num_profiles"])
build_flags.define("NUM_LAYERS", kb["num_layers"])
build_flags.define("NUM_KEYS", kb["num_keys"])
build_flags.define("NUM_ADVANCED_KEYS", kb["num_advanced_keys"])

# Default Keymap
build_flags.define("DEFAULT_KEYMAP", utils.to_c_array(kb_json["keymap"]))

# Actuation Configuration
if "actuation" in kb_json:
    actuation = kb_json["actuation"]
    if "actuation_point" in actuation:
        build_flags.define("ACTUATION_POINT", actuation["actuation_point"])

# Add source build flags
env.Append(BUILD_FLAGS=build_flags.get_flags())
