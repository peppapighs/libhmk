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
  // Configure flash PSR register
  flash_psr_set(FLASH_WAIT_CYCLE_6);
  // Enable PWC peripheral clock
  crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);
  // Set power LDO output voltage to 1.3V
  pwc_ldo_output_voltage_set(PWC_LDO_OUTPUT_1V3);
  // Set clock source to HSE
  crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);

  // Wait for HSE to stabilize
  while (crm_hext_stable_wait() == ERROR)
    ;

  // Configure PLL
  crm_pll_config(CRM_PLL_SOURCE_HEXT, 72, BOARD_HSE_VALUE / 1000000,
                 CRM_PLL_FP_4);
  // Configure USB clock divider
  crm_pllu_div_set(CRM_PLL_FU_18);
  // Enable PLL
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
  // Enable auto step mode
  crm_auto_step_mode_enable(TRUE);
  // Select PLL as system clock source
  crm_sysclk_switch(CRM_SCLK_PLL);

  // Wait for system clock to switch
  while (crm_sysclk_switch_status_get() != CRM_SCLK_PLL)
    ;

  // Disable auto step mode
  crm_auto_step_mode_enable(FALSE);
  // Update system core clock
  system_core_clock_update();
}

#if !defined(BOARD_USB_HS)
/**
 * @brief Reduce power consumption of USB HS PHY when not initialized (FAQ0148)
 *
 * @return None
 */
static void board_reduce_power_consumption(void) {
  volatile uint32_t delay = 0x34BC0;

  if (CRM->ctrl_bit.hextstbl) {
    *(__IO uint32_t *)0x40023878 = 0x00;
  } else if (CRM->ctrl_bit.pllstbl == SET) {
    CRM->pllcfg_bit.plluen = TRUE;
    while (CRM->ctrl_bit.pllstbl != SET || CRM->ctrl_bit.pllustbl != SET)
      ;
    *(__IO uint32_t *)0x40023878 = 0x10;
  } else {
    return;
  }
  CRM->ahben1 |= 1 << 29;
  *(__IO uint32_t *)0x40040038 = 0x210000;
  *(__IO uint32_t *)0x4004000C |= 0x40000000;
  *(__IO uint32_t *)0x40040804 &= ~0x2;
  while (delay--) {
    if (*(__IO uint32_t *)0x40040808 & 0x1)
      break;
  }
  *(__IO uint32_t *)0x40040038 |= 0x400000;
  *(__IO uint32_t *)0x40040E00 |= 0x1;
  *(__IO uint32_t *)0x40040038 &= ~0x10000;
}
#endif

/**
 * @brief Initialize the USB
 *
 * @return None
 */
static void board_usb_init(void) {
#if !defined(BOARD_USB_HS)
  board_reduce_power_consumption();
#endif

#if defined(BOARD_USB_FS)
  // Configure USB FS clock
  crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_OTGFS1_PERIPH_CLOCK, TRUE);
#elif defined(BOARD_USB_HS)
  // Configure USB HS clock
  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_OTGHS_PERIPH_CLOCK, TRUE);
#else
#error "USB peripheral not defined"
#endif

  // Configure PLLU for USB
  crm_pllu_output_set(TRUE);

  // Wait for PLLU output to stabilize
  while (crm_flag_get(CRM_PLLU_STABLE_FLAG) != SET)
    ;

  // Configure USB clock source to PLLU
  crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_PLLU);

#if defined(BOARD_USB_FS)
  // Set NVIC priority for USB FS interrupt
  NVIC_SetPriority(OTGFS1_IRQn, 0);
#elif defined(BOARD_USB_HS)
  // Set NVIC priority for USB HS interrupt
  NVIC_SetPriority(OTGHS_IRQn, 0);
#else
#error "USB peripheral not defined"
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

uint32_t board_serial(uint16_t *buf) {
  static char serial_str[24 + 1];

  const uint32_t *uid = (const uint32_t *)(0x1FFFF7E8);
  // Use the 96-bit unique ID as the serial number
  sprintf(serial_str, "%08" PRIX32 "%08" PRIX32 "%08" PRIX32, uid[2], uid[1],
          uid[0]);

  // Convert ASCII string into UTF-16-LE
  for (size_t i = 0; i < 24; i++)
    buf[i] = serial_str[i];

  return 24;
}

uint32_t board_cycle_count(void) { return DWT->CYCCNT; }

//--------------------------------------------------------------------+
// Interrupt Handlers
//--------------------------------------------------------------------+

void OTGFS1_IRQHandler(void) { tud_int_handler(0); }

void OTGFS1_WKUP_IRQHandler(void) { tud_int_handler(0); }

void OTGHS_IRQHandler(void) { tud_int_handler(1); }

void OTGHS_WKUP_IRQHandler(void) { tud_int_handler(1); }
