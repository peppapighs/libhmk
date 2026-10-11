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

#include "hardware/hardware.h"

#include "stm32f7xx_hal.h"
#include "tusb.h"

static void board_clock_init(void) {
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState = RCC_HSE_ON;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  // Keep the VCO input at 2 MHz. With the KBHE 16 MHz HSE this produces a
  // 432 MHz VCO, a 216 MHz CPU clock and a valid 48 MHz PLLQ output.
  osc.PLL.PLLM = BOARD_HSE_VALUE / 2000000u;
  osc.PLL.PLLN = 216;
  osc.PLL.PLLP = RCC_PLLP_DIV2;
  osc.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    board_error_handler();

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    board_error_handler();

  clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV4;
  clk.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_7) != HAL_OK)
    board_error_handler();
}

static void board_usb_init(void) {
  GPIO_InitTypeDef gpio = {0};

#if defined(BOARD_USB_FS)
  RCC_PeriphCLKInitTypeDef periph_clk = {0};

  // The FS PHY uses the 48 MHz PLLQ output, not the integrated HS PHY PLL.
  periph_clk.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
  periph_clk.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&periph_clk) != HAL_OK)
    board_error_handler();

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

  gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &gpio);

  // Bus-powered device: VBUS sensing is disabled, as in the F446 driver.
  USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBDEN;
  USB_OTG_FS->GOTGCTL |= USB_OTG_GOTGCTL_BVALOEN;
  USB_OTG_FS->GOTGCTL |= USB_OTG_GOTGCTL_BVALOVAL;

  HAL_NVIC_SetPriority(OTG_FS_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
#elif defined(BOARD_USB_HS)
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_OTGPHYC_CLK_ENABLE();
  __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
  // Required by the DWC2 core reset path even when the embedded PHY is used.
  __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();

#if defined(RCC_AHB1LPENR_OTGHSULPILPEN)
  RCC->AHB1LPENR &= ~RCC_AHB1LPENR_OTGHSULPILPEN;
#endif

  gpio.Pin = GPIO_PIN_14 | GPIO_PIN_15;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF12_OTG_HS_FS;
  HAL_GPIO_Init(GPIOB, &gpio);

  // KBHE does not route the optional PB13 VBUS-sense input. Advertise a valid
  // B-device session in software, as required by TinyUSB's STM32F7 BSP.
  USB_OTG_HS->GCCFG &= ~USB_OTG_GCCFG_VBDEN;
  USB_OTG_HS->GOTGCTL |= USB_OTG_GOTGCTL_BVALOEN;
  USB_OTG_HS->GOTGCTL |= USB_OTG_GOTGCTL_BVALOVAL;

  HAL_NVIC_SetPriority(OTG_HS_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
#else
#error "USB peripheral not defined"
#endif
}

static void board_bootloader_jump(void) {
  volatile const uint32_t *vector =
      (volatile const uint32_t *)BOOTLOADER_ADDR;
  const uint32_t sp = vector[0];
  const uint32_t entry = vector[1];

  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  for (uint32_t i = 0; i < M_ARRAY_SIZE(NVIC->ICER); i++)
    NVIC->ICER[i] = UINT32_MAX;

  SCB->VTOR = (uint32_t)BOOTLOADER_ADDR;
  __set_MSP(sp);
  __set_PSP(sp);
  ((void (*)(void))entry)();
  while (1)
    ;
}

extern uint32_t _board_bootloader_flag[];
#define BOARD_BOOTLOADER_FLAG _board_bootloader_flag[0]

void board_init(void) {
  if (BOARD_BOOTLOADER_FLAG == BOOTLOADER_MAGIC) {
    BOARD_BOOTLOADER_FLAG = 0;
    board_bootloader_jump();
  }

  // Match TinyUSB's STM32F7 policy: instruction cache materially reduces scan
  // time, while data cache stays disabled so DMA buffers are coherent by
  // default. Explicitly disable a cache that a chainload bootloader may have
  // left enabled; CMSIS cleans it before switching it off. The ADC backend
  // still performs aligned maintenance if D-cache is enabled later.
  if ((SCB->CCR & SCB_CCR_DC_Msk) != 0u)
    SCB_DisableDCache();
  SCB_EnableICache();

  HAL_Init();
  board_clock_init();
  board_usb_init();

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
}

void board_error_handler(void) {
  __disable_irq();
  while (1)
    ;
}

void board_reset(void) { NVIC_SystemReset(); }

void board_enter_bootloader(void) {
  BOARD_BOOTLOADER_FLAG = BOOTLOADER_MAGIC;
  NVIC_SystemReset();
}

uint32_t board_serial(char *buf) {
  const volatile uint8_t *uid = (const volatile uint8_t *)UID_BASE;
  for (uint32_t i = 0; i < 12; i++) {
    buf[i * 2u] = M_HEX(uid[i] >> 4);
    buf[i * 2u + 1u] = M_HEX(uid[i] & 0x0fu);
  }
  return 24;
}

uint32_t board_cycle_count(void) { return DWT->CYCCNT; }

uint32_t tusb_time_millis_api(void) { return HAL_GetTick(); }

void tusb_time_delay_ms_api(uint32_t ms) { HAL_Delay(ms); }

void SysTick_Handler(void) { HAL_IncTick(); }

#if defined(BOARD_USB_FS)
void OTG_FS_IRQHandler(void) { tud_int_handler(BOARD_TUD_RHPORT); }
#elif defined(BOARD_USB_HS)
void OTG_HS_IRQHandler(void) { tud_int_handler(BOARD_TUD_RHPORT); }
#endif
