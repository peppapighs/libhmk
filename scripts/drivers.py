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


from dataclasses import dataclass
from typing import Callable


@dataclass
class PlatformIO:
    # PlatformIO board identifier
    board: str
    # Linker script used for the board
    ldscript: str
    # PlatformIO framework identifier
    framework: str
    # PlatformIO platform identifier
    platform: str


@dataclass
class TinyUSB:
    # TinyUSB MCU identifier
    mcu: str


@dataclass
class Bootloader:
    # Flash address of the bootloader
    address: int
    # Magic value to enter bootloader on firmware startup
    magic: int


@dataclass
class NonUniformSectors:
    # Size of each flash sector
    sizes: list[int]


@dataclass
class UniformSectors:
    # Common sector size
    size: int
    # Number of flash sectors
    num_sectors: int


@dataclass
class Flash:
    sector_sizes: NonUniformSectors | UniformSectors
    # Value of an empty word in the flash
    empty_value: int

    # Get the number of flash sectors
    def get_num_sectors(self):
        match self.sector_sizes:
            case NonUniformSectors(sizes):
                return len(sizes)
            case UniformSectors(_, num_sectors):
                return num_sectors

    # Get the total size of the flash
    def get_flash_size(self):
        match self.sector_sizes:
            case NonUniformSectors(sizes):
                return sum(sizes)
            case UniformSectors(size, num_sectors):
                return size * num_sectors

    # Get the size of a flash sector
    def get_sector_size(self, sector: int):
        match self.sector_sizes:
            case NonUniformSectors(sizes):
                assert sector < len(sizes)
                return sizes[sector]
            case UniformSectors(size, num_sectors):
                assert sector < num_sectors
                return size

    # Round up the required size to the minimum number of flash sectors (from the end)
    def round_up_to_flash_sectors(self, required_size: int):
        assert required_size <= self.get_flash_size()
        total = 0
        for i in range(self.get_num_sectors() - 1, -1, -1):
            total += self.get_sector_size(i)
            if total >= required_size:
                return total
        return total


@dataclass
class ADC:
    # Maxmimum ADC resolution supported by the MCU
    max_resolution: int
    # ADC input pin names, in the same order as the ADC channels
    input_pins: list[str]
    # Convert a list of ADC input pin names to a tuple of (GPIO port names, GPIO pin numbers)
    to_gpio_array: Callable[[list[str]], tuple[list[str], list[str]]]

    # Convert a list of ADC input channels or GPIO pin names to a list of ADC input channels
    def to_adc_inputs(self, channels_or_pins: list[str | int]):
        channels: list[int] = []
        for x in channels_or_pins:
            match x:
                case str():
                    channels.append(self.input_pins.index(x))
                case int():
                    assert x < len(self.input_pins)
                    channels.append(x)
        return channels


@dataclass
class Metadata:
    bootloader: Bootloader
    flash: Flash
    adc: ADC


@dataclass
class Driver:
    platformio: PlatformIO
    tinyusb: TinyUSB
    metadata: Metadata


STM32F446XX = Driver(
    platformio=PlatformIO(
        board="genericSTM32F446RE",
        ldscript="stm32f446retx.ld",
        framework="stm32cube",
        platform="ststm32",
    ),
    tinyusb=TinyUSB(mcu="stm32f4"),
    metadata=Metadata(
        bootloader=Bootloader(
            address=0x1FFF0000,
            magic=0xDEADBEEF,
        ),
        flash=Flash(
            sector_sizes=NonUniformSectors(
                sizes=[
                    16 * 1024,
                    16 * 1024,
                    16 * 1024,
                    16 * 1024,
                    64 * 1024,
                    128 * 1024,
                    128 * 1024,
                    128 * 1024,
                ]
            ),
            empty_value=0xFFFFFFFF,
        ),
        adc=ADC(
            max_resolution=12,
            input_pins=[
                "A0",
                "A1",
                "A2",
                "A3",
                "A4",
                "A5",
                "A6",
                "A7",
                "B0",
                "B1",
                "C0",
                "C1",
                "C2",
                "C3",
                "C4",
                "C5",
            ],
            to_gpio_array=lambda pins: (
                [f"GPIO{pin[0]}" for pin in pins],
                [f"GPIO_PIN_{pin[1:]}" for pin in pins],
            ),
        ),
    ),
)

AT32F405XX = Driver(
    platformio=PlatformIO(
        board="genericAT32F405RCT7",
        ldscript="at32f405xc.ld",
        framework="at32firmlib",
        platform="https://github.com/ArteryTek/platform-arterytekat32.git",
    ),
    tinyusb=TinyUSB(mcu="at32f402_405"),
    metadata=Metadata(
        bootloader=Bootloader(
            address=0x1FFFA400,
            magic=0xDEADBEEF,
        ),
        flash=Flash(
            sector_sizes=UniformSectors(
                size=2048,
                num_sectors=128,
            ),
            empty_value=0xFFFFFFFF,
        ),
        adc=ADC(
            max_resolution=12,
            input_pins=[
                "A0",
                "A1",
                "A2",
                "A3",
                "A4",
                "A5",
                "A6",
                "A7",
                "B0",
                "B1",
                "C0",
                "C1",
                "C2",
                "C3",
                "C4",
                "C5",
            ],
            to_gpio_array=lambda pins: (
                [f"GPIO{pin[0]}" for pin in pins],
                [f"GPIO_PINS_{pin[1:]}" for pin in pins],
            ),
        ),
    ),
)
