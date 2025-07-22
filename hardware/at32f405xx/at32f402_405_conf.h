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

#pragma once

#include "common.h"

//--------------------------------------------------------------------+
// Crystal Configuration
//--------------------------------------------------------------------+

/// High-speed external crystal frequency
#define HEXT_VALUE ((uint32_t)BOARD_HSE_VALUE)
// High-speed internal clock frequency
#define HICK_VALUE ((uint32_t)8000000)

// Time out for high-speed external crystal startup
#define HEXT_STARTUP_TIMEOUT ((uint16_t)0x3000)

//--------------------------------------------------------------------+
// Module Configuration
//--------------------------------------------------------------------+

#define ACC_MODULE_ENABLED
#define ADC_MODULE_ENABLED
#define CAN_MODULE_ENABLED
#define CRC_MODULE_ENABLED
#define CRM_MODULE_ENABLED
#define DEBUG_MODULE_ENABLED
#define DMA_MODULE_ENABLED
#define ERTC_MODULE_ENABLED
#define EXINT_MODULE_ENABLED
#define FLASH_MODULE_ENABLED
#define GPIO_MODULE_ENABLED
#define I2C_MODULE_ENABLED
#define MISC_MODULE_ENABLED
#define PWC_MODULE_ENABLED
#define QSPI_MODULE_ENABLED
#define SCFG_MODULE_ENABLED
#define SPI_MODULE_ENABLED
#define TMR_MODULE_ENABLED
#define USART_MODULE_ENABLED
#define USB_MODULE_ENABLED
#define WDT_MODULE_ENABLED
#define WWDT_MODULE_ENABLED

//--------------------------------------------------------------------+
// Header Files
//--------------------------------------------------------------------+

#ifdef ACC_MODULE_ENABLED
#include "at32f402_405_acc.h"
#endif

#ifdef ADC_MODULE_ENABLED
#include "at32f402_405_adc.h"
#endif

#ifdef CAN_MODULE_ENABLED
#include "at32f402_405_can.h"
#endif

#ifdef CRC_MODULE_ENABLED
#include "at32f402_405_crc.h"
#endif

#ifdef CRM_MODULE_ENABLED
#include "at32f402_405_crm.h"
#endif

#ifdef DEBUG_MODULE_ENABLED
#include "at32f402_405_debug.h"
#endif

#ifdef DMA_MODULE_ENABLED
#include "at32f402_405_dma.h"
#endif

#ifdef ERTC_MODULE_ENABLED
#include "at32f402_405_ertc.h"
#endif

#ifdef EXINT_MODULE_ENABLED
#include "at32f402_405_exint.h"
#endif

#ifdef FLASH_MODULE_ENABLED
#include "at32f402_405_flash.h"
#endif

#ifdef GPIO_MODULE_ENABLED
#include "at32f402_405_gpio.h"
#endif

#ifdef I2C_MODULE_ENABLED
#include "at32f402_405_i2c.h"
#endif

#ifdef MISC_MODULE_ENABLED
#include "at32f402_405_misc.h"
#endif

#ifdef PWC_MODULE_ENABLED
#include "at32f402_405_pwc.h"
#endif

#ifdef QSPI_MODULE_ENABLED
#include "at32f402_405_qspi.h"
#endif

#ifdef SCFG_MODULE_ENABLED
#include "at32f402_405_scfg.h"
#endif

#ifdef SPI_MODULE_ENABLED
#include "at32f402_405_spi.h"
#endif

#ifdef TMR_MODULE_ENABLED
#include "at32f402_405_tmr.h"
#endif

#ifdef USART_MODULE_ENABLED
#include "at32f402_405_usart.h"
#endif

#ifdef USB_MODULE_ENABLED
#include "at32f402_405_usb.h"
#endif

#ifdef WDT_MODULE_ENABLED
#include "at32f402_405_wdt.h"
#endif

#ifdef WWDT_MODULE_ENABLED
#include "at32f402_405_wwdt.h"
#endif
