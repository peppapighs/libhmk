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

from enum import Enum
from pydantic import (
    BaseModel,
    Field,
    NonNegativeInt,
    PositiveFloat,
    PositiveInt,
    model_validator,
)
from typing import Annotated


RGBComponent = Annotated[int, Field(ge=0, le=255)]
LEDIndex = Annotated[int, Field(ge=0, le=254)]


class KeyboardUSBPort(str, Enum):
    FULL_SPEED = "fs"
    HIGH_SPEED = "hs"


# USB Configuration
class KeyboardUSB(BaseModel):
    # USB Vendor ID
    vid: str = Field(pattern=r"^0x[0-9A-Fa-f]{4}$")
    # USB Product ID
    pid: str = Field(pattern=r"^0x[0-9A-Fa-f]{4}$")
    port: KeyboardUSBPort


# Keyboard Configuration
class KeyboardKeyboard(BaseModel):
    num_profiles: int = Field(ge=1, le=8)
    num_layers: int = Field(ge=1, le=8)
    num_keys: int = Field(ge=1, le=256)
    num_advanced_keys: int = Field(ge=1, le=64)
    # Maximum number of Dynamic Keystroke bindings per key. Higher values may require higher storage sizes.
    num_dynamic_keystroke_max_bindings: int = Field(ge=4, le=64, default=4)
    num_macro_nodes: int = Field(ge=1, le=255, default=128)


# Hardware Configuration
class KeyboardHardware(BaseModel):
    # High-speed external oscillator value in Hz
    hse_value: PositiveInt
    # Keyboard driver name
    driver: str


# Raw ADC input configuration
class KeyboardAnalogRaw(BaseModel):
    # Array of raw ADC input channels. If a string is provided, it is used as the GPIO pin name
    input: list[NonNegativeInt | str]
    # Array of key index mappings for the raw ADC inputs
    vector: list[NonNegativeInt]


# Analog Multiplexer ADC input configuration
class KeyboardAnalogMux(BaseModel):
    # Array of GPIO pin names for the multiplexer select lines
    select: list[str]
    # Array of ADC input channels for the multiplexers. If a string is provided, it is used as the GPIO pin name
    input: list[NonNegativeInt | str]
    # Mapping from multiplexers to ADC input channels
    matrix: list[list[NonNegativeInt]]


# Analog Configuration
class KeyboardAnalog(BaseModel):
    # ADC resolution for this keyboard. A higher value means higher accuracy but slower matrix scans. Default to the maximum resolution supported by the MCU if not provided.
    adc_resolution: PositiveInt | None = None
    # Set to true if the ADC value is inversely proportional to the travel distance of the keys
    invert_adc: bool = False
    # Delay in microseconds between ADC scans
    delay: int | None = None
    raw: KeyboardAnalogRaw | None = None
    mux: KeyboardAnalogMux | None = None


# Calibration Configuration
class KeyboardCalibration(BaseModel):
    # See `include/lib/eeconfig.h`
    initial_rest_value: NonNegativeInt
    # See `include/lib/eeconfig.h`
    initial_bottom_out_threshold: NonNegativeInt


# Wear leveling Configuration
class KeyboardWearLeveling(BaseModel):
    # Size of the virtual persistent storage in bytes. There must be enough RAM of this size to hold the entire virtual storage.
    virtual_size: int = Field(ge=1, le=16384)
    # Size of the write log in bytes
    write_log_size: int = Field(ge=1, le=65536)


class KeyboardLayoutKey(BaseModel):
    key: NonNegativeInt
    w: PositiveFloat | None = None
    h: PositiveFloat | None = None
    x: float | None = None
    y: float | None = None
    # Option key and value pairs for the corresponding labels
    option: tuple[int, int] | None = None


# Keyboard Layout
class KeyboardLayout(BaseModel):
    # Labels for each layout option. Use a string for a toggle option, or an array for a select option
    labels: list[str | list[str]] | None = None
    # Metadata for how the keyboard should be rendered in the web configurator
    keymap: list[list[KeyboardLayoutKey]]


# Actuation Configuration
class KeyboardActuation(BaseModel):
    # Default actuation point
    actuation_point: int = Field(ge=0, le=255)


class KeyboardRGB(BaseModel):
    # Number of addressable RGB LEDs. The bridge protocol encodes this in one byte.
    num_leds: int = Field(ge=1, le=255)
    # GPIO pin used by the board-specific non-blocking backend.
    data_pin: str = Field(pattern=r"^A(?:0|5|15)$")
    # Currently supported by the STM32F723 driver used by KBHE.
    backend: str = Field(pattern=r"^ws2812_tim2_ch1$")
    default_brightness: int = Field(ge=0, le=255, default=50)
    default_color: tuple[RGBComponent, RGBComponent, RGBComponent] = (255, 255, 255)
    # Wiring order: `led_index_map[logical] = position in the LED chain`. Boards
    # whose strip snakes across the PCB declare it so the core, the effects and
    # the host protocol all address LEDs in one stable logical order. Omitting
    # it means the chain order is the logical order.
    led_index_map: list[LEDIndex] | None = None
    # Physical `[x, y]` of each logical LED in any consistent integer unit
    # (KBHE uses 0.1 mm from the PCB placement). Position-aware effects such as
    # the rainbow wave sweep along these axes instead of along the chain.
    led_position: list[tuple[int, int]] | None = None
    # Which LED lights each key, in logical key order. `null` marks a key with
    # no LED. Hosts must not infer this from matching key and LED counts.
    key_to_led: list[LEDIndex | None] | None = None

    @model_validator(mode="after")
    def validate_topology(self):
        if self.led_index_map is not None:
            if len(self.led_index_map) != self.num_leds:
                raise ValueError(
                    f"Expected led_index_map to have {self.num_leds} entries"
                )
            if sorted(self.led_index_map) != list(range(self.num_leds)):
                raise ValueError(
                    "led_index_map must be a permutation of the LED chain"
                )
        if self.led_position is not None:
            if len(self.led_position) != self.num_leds:
                raise ValueError(
                    f"Expected led_position to have {self.num_leds} entries"
                )
            if any(x < 0 or y < 0 for x, y in self.led_position):
                raise ValueError("led_position coordinates must not be negative")
        if self.key_to_led is not None and any(
            led is not None and led >= self.num_leds for led in self.key_to_led
        ):
            raise ValueError(
                f"key_to_led entries must be below {self.num_leds}"
            )
        return self


# keyboard.json Schema
class Keyboard(BaseModel):
    name: str
    manufacturer: str
    maintainer: str
    usb: KeyboardUSB
    keyboard: KeyboardKeyboard
    hardware: KeyboardHardware
    analog: KeyboardAnalog
    calibration: KeyboardCalibration
    wear_leveling: KeyboardWearLeveling | None = None
    layout: KeyboardLayout
    # Default keymap
    keymap: list[list[str]] | None = None
    # Default keymaps for each profile. If not specified, the default keymap will be used for all profiles.
    keymaps: list[list[list[str]]] | None = None
    actuation: KeyboardActuation | None = None
    rgb: KeyboardRGB | None = None
