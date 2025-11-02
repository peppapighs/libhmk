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

#include "at32f402_405.h"
#include "tusb.h"

/**
 * @brief Initialize the clock
 *
 * @return None
 */
static void board_clock_init(void) {
  // Reset the CRM
  crm_reset();
  // Configure flash PSR register with 6 wait states for 216MHz system clock
  flash_psr_set(FLASH_WAIT_CYCLE_6);
  // Enable PWC peripheral clock
  crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);
  // Set power LDO output voltage to 1.3V to support 216MHz system clock
  pwc_ldo_output_voltage_set(PWC_LDO_OUTPUT_1V3);
  // Set clock source to HSE
  crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);

  // Wait for HSE to stabilize
  while (crm_hext_stable_wait() == ERROR)
    ;

  // Configure PLL to 216MHz, assuming HSE is 12MHz
  crm_pll_config(CRM_PLL_SOURCE_HEXT, 72, 1, CRM_PLL_FP_4);
  // Configure PLLU to 48MHz for USB FS
  crm_pllu_div_set(CRM_PLL_FU_18);
  // Enable PLL as system clock source
  crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);

  // Wait for PLL to stabilize
  while (crm_flag_get(CRM_PLL_STABLE_FLAG) != SET)
    ;

  // Configure AHB clock
  crm_ahb_div_set(CRM_AHB_DIV_1);
  // Configure APB2 clock
  crm_apb2_div_set(CRM_APB2_DIV_1);
  // Configure APB1 clock
  crm_apb1_div_set(CRM_APB1_DIV_2);
  // Enable auto step mode before switching system clock source to PLL
  crm_auto_step_mode_enable(TRUE);
  // Select PLL as system clock source
  crm_sysclk_switch(CRM_SCLK_PLL);

  // Wait for system clock to finish switching
  while (crm_sysclk_switch_status_get() != CRM_SCLK_PLL)
    ;

  // Disable auto step mode after switching system clock source
  crm_auto_step_mode_enable(FALSE);
  // Update system core clock variable
  system_core_clock_update();
}

#if defined(BOARD_USB_FS)
/**
 * @brief Reduce power consumption of USB HS PHY when not initialized (FAQ0148)
 *
 * @return None
 */
static void board_reduce_power_consumption(void) {
  volatile uint32_t delay = F_CPU / 1000;

  // Wait for HSE to stabilize
  while (crm_hext_stable_wait() == ERROR)
    ;

  // Configure HS PHY clock source to HSE to support power down
  crm_usb_phy12_clock_select(CRM_USB_PHY12_CLOCK_HEXT_DIV_1);
  crm_periph_clock_enable(CRM_OTGHS_PERIPH_CLOCK, TRUE);

  // Enable power down mode
  OTG2_GLOBAL->gccfg_bit.pwrdown = TRUE;
  OTG2_GLOBAL->gccfg_bit.vbusig = TRUE;

  // Set USB mode to device
  usb_global_set_mode(OTG2_GLOBAL, OTG_DEVICE_MODE);
  usb_connect(OTG2_GLOBAL);

  // Roughly 1ms delay to enter suspend mode
  while (delay--) {
    if (usb_suspend_status_get(OTG2_GLOBAL) == SET)
      break;
  }

  // Wait for HS PHY clock source to stabilize
  OTG2_GLOBAL->gccfg_bit.wait_clk_rcv = TRUE;
  // Stop USB PHY clock to reduce power consumption
  usb_stop_phy_clk(OTG2_GLOBAL);
  // Disable power down mode
  OTG2_GLOBAL->gccfg_bit.pwrdown = FALSE;
}
#endif

#if defined(BOARD_USB_FS)
static otg_global_type *otg_global = OTG1_GLOBAL;
#elif defined(BOARD_USB_HS)
static otg_global_type *otg_global = OTG2_GLOBAL;
#else
#error "USB peripheral not defined"
#endif

/**
 * @brief Initialize the USB
 *
 * @return None
 */
static void board_usb_init(void) {
#if defined(BOARD_USB_FS)
  board_reduce_power_consumption();

  // Configure USB FS clock
  crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_OTGFS1_PERIPH_CLOCK, TRUE);
#elif defined(BOARD_USB_HS)
  // Configure USB HS clock
  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_OTGHS_PERIPH_CLOCK, TRUE);
#endif

  // Configure PLLU for USB
  crm_pllu_output_set(TRUE);

  // Wait for PLLU output to stabilize
  while (crm_flag_get(CRM_PLLU_STABLE_FLAG) != SET)
    ;

  // Configure USB clock source to PLLU
  crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_PLLU);

  // Ignore USB FS VBUS sensing
  otg_global->gccfg_bit.vbusig = TRUE;
#if defined(BOARD_USB_FS)
  // Set NVIC priority for USB FS interrupt
  NVIC_SetPriority(OTGFS1_IRQn, 0);
#elif defined(BOARD_USB_HS)
  // Set NVIC priority for USB HS interrupt
  NVIC_SetPriority(OTGHS_IRQn, 0);
#endif
}

static void board_bootloader_jump(void) {
  volatile const uint32_t *bootloader_vector =
      (volatile const uint32_t *)BOOTLOADER_ADDR;
  uint32_t sp = bootloader_vector[0];
  uint32_t bootloader_entry = bootloader_vector[1];

  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  // Disable all interrupts
  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICER[1] = 0xFFFFFFFF;
  NVIC->ICER[2] = 0xFFFFFFFF;
  NVIC->ICER[3] = 0xFFFFFFFF;

  SCB->VTOR = (uint32_t)BOOTLOADER_ADDR;

  // Set stack pointer
  __set_MSP(sp);
  __set_PSP(sp);

  ((void (*)(void))bootloader_entry)();
  while (1)
    ;
}

extern uint32_t _board_bootloader_flag[];
#define BOARD_BOOTLOADER_FLAG _board_bootloader_flag[0]

void board_init(void) {
  // We need to enter the bootloader before the clock peripheral is initialized
  // since it is initialized differently in the bootloader.
  if (BOARD_BOOTLOADER_FLAG == BOOTLOADER_MAGIC) {
    // Clear the bootloader flag before jumping to the bootloader
    BOARD_BOOTLOADER_FLAG = 0;
    board_bootloader_jump();
  }

  // Configure NVIC priority group
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);

  board_clock_init();
  board_usb_init();

  // Enable cycle counter
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
  // Set the bootloader flag
  BOARD_BOOTLOADER_FLAG = BOOTLOADER_MAGIC;

  // Reset the board to enter the bootloader
  NVIC_SystemReset();
}

uint32_t board_serial(char *buf) {
  const volatile uint8_t *uid = (const volatile uint8_t *)(0x1FFFF7E8);
  // Use the 96-bit unique ID as the serial number
  for (uint32_t i = 0; i < 12; i++) {
    buf[i * 2] = M_HEX(uid[i] >> 4);
    buf[i * 2 + 1] = M_HEX(uid[i] & 0x0F);
  }

  return 24;
}

uint32_t board_cycle_count(void) { return DWT->CYCCNT; }

//--------------------------------------------------------------------+
// Interrupt Handlers
//--------------------------------------------------------------------+

// AT32F405 has a weird behavior, where if it has an unstable D+/D- during
// startup, there will be a pending suspend interrupt before enumeration is
// complete. This caused TinyUSB internal state to get stuck in a suspend state.
// The issue was fixed in https://github.com/hathach/tinyusb/pull/3319.

void OTGFS1_IRQHandler(void) { tud_int_handler(0); }

void OTGHS_IRQHandler(void) { tud_int_handler(1); }

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

void tud_suspend_cb(bool remote_wakeup_en) {
  // Stop PHY clock on suspend to save power
  usb_stop_phy_clk(otg_global);
}

void tud_resume_cb(void) { usb_open_phy_clk(otg_global); }
