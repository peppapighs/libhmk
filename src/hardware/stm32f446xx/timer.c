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

#include "stm32f4xx_hal.h"

#define BUZZER_GPIO_PORT GPIOC
#define BUZZER_GPIO_PIN GPIO_PIN_6
#define BUZZER_GPIO_AF GPIO_AF2_TIM3

#define BUZZER_TIM_INSTANCE TIM3
#define BUZZER_TIM_CHANNEL TIM_CHANNEL_1

#define BUZZER_DMA_INSTANCE DMA1_Stream2
#define BUZZER_DMA_CHANNEL DMA_CHANNEL_5
#define BUZZER_DMA_IRQN DMA1_Stream2_IRQn

#define BUZZER_SINE_TABLE_SIZE 128U
#define BUZZER_TARGET_FREQUENCY_HZ 1000U
#define BUZZER_TIM_CLOCK_HZ 90000000U 
// TIM3 on APB1: 16 MHz HSE -> 180 MHz SYSCLK, APB1 divider 4,
// timers on divided APB double their clock => 90 MHz

static TIM_HandleTypeDef buzzer_tim = {0};
static DMA_HandleTypeDef buzzer_dma = {0};
static uint16_t buzzer_waveform[BUZZER_SINE_TABLE_SIZE];
static bool buzzer_initialized = false;
static bool buzzer_running = false;

// 16-bit sine wave scaled to 0..65535 to simplify runtime scaling.
static const uint16_t buzzer_sine_template[BUZZER_SINE_TABLE_SIZE] = {
    32768, 34375, 35979, 37575, 39160, 40729, 42279, 43807, 45307, 46777, 48214,
    49613, 50972, 52287, 53555, 54773, 55938, 57047, 58097, 59087, 60013, 60873,
    61666, 62389, 63041, 63620, 64124, 64553, 64905, 65180, 65377, 65496, 65535,
    65496, 65377, 65180, 64905, 64553, 64124, 63620, 63041, 62389, 61666, 60873,
    60013, 59087, 58097, 57047, 55938, 54773, 53555, 52287, 50972, 49613, 48214,
    46777, 45307, 43807, 42279, 40729, 39160, 37575, 35979, 34375, 32768, 31160,
    29556, 27960, 26375, 24806, 23256, 21728, 20228, 18758, 17321, 15922, 14563,
    13248, 11980, 10762, 9597, 8488, 7438, 6448, 5522, 4662, 3869, 3146, 2494,
    1915, 1411, 982, 630, 355, 158, 39, 0, 39, 158, 355, 630, 982, 1411, 1915,
    2494, 3146, 3869, 4662, 5522, 6448, 7438, 8488, 9597, 10762, 11980, 13248,
    14563, 15922, 17321, 18758, 20228, 21728, 23256, 24806, 26375, 27960, 29556,
    31160};

static void buzzer_build_waveform(uint32_t period) {
  for (uint32_t i = 0; i < BUZZER_SINE_TABLE_SIZE; i++) {
    buzzer_waveform[i] =
        (uint16_t)(((uint32_t)buzzer_sine_template[i] * period) / 65535U);
  }
}

static void buzzer_gpio_init(void) {
  GPIO_InitTypeDef gpio_init = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();

  gpio_init.Pin = BUZZER_GPIO_PIN;
  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init.Alternate = BUZZER_GPIO_AF;
  HAL_GPIO_Init(BUZZER_GPIO_PORT, &gpio_init);
}

static void buzzer_dma_init(void) {
  __HAL_RCC_DMA1_CLK_ENABLE();

  buzzer_dma.Instance = BUZZER_DMA_INSTANCE;
  buzzer_dma.Init.Channel = BUZZER_DMA_CHANNEL;
  buzzer_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
  buzzer_dma.Init.PeriphInc = DMA_PINC_DISABLE;
  buzzer_dma.Init.MemInc = DMA_MINC_ENABLE;
  buzzer_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  buzzer_dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  buzzer_dma.Init.Mode = DMA_CIRCULAR;
  buzzer_dma.Init.Priority = DMA_PRIORITY_HIGH;
  buzzer_dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&buzzer_dma) != HAL_OK)
    board_error_handler();

  __HAL_LINKDMA(&buzzer_tim, hdma[TIM_DMA_ID_UPDATE], buzzer_dma);

  HAL_NVIC_SetPriority(BUZZER_DMA_IRQN, 5, 0);
  HAL_NVIC_EnableIRQ(BUZZER_DMA_IRQN);
}

static void buzzer_timer_init(void) {
  uint32_t tim_clk = BUZZER_TIM_CLOCK_HZ;
  uint32_t target_sample_rate = (uint32_t)BUZZER_TARGET_FREQUENCY_HZ * BUZZER_SINE_TABLE_SIZE;

  if (target_sample_rate == 0U || tim_clk == 0U)
    board_error_handler();

  // Find prescaler/period pair that keeps ARR within 16 bits while keeping
  // the sample rate close to the target.
  uint32_t ideal_counts = (tim_clk + target_sample_rate / 2U) / target_sample_rate;

  if (ideal_counts < 2U) {
    ideal_counts = 2U;
  }

  uint32_t prescaler = (ideal_counts + 65535U) / 65536U;
  if (prescaler == 0U) {
    prescaler = 1U;
  }

  uint32_t counts_per_sample = (ideal_counts + prescaler / 2U) / prescaler;

  if (counts_per_sample < 2U) {
    counts_per_sample = 2U;
  }

  uint32_t period = counts_per_sample - 1U;
  uint32_t prescaler_reg = prescaler - 1U;

  // `counts_per_sample` may not be a perfect match, but the resulting tone will
  // be within a few hertz of `BUZZER_TARGET_FREQUENCY_HZ` for the current clock
  // tree configuration.

  buzzer_build_waveform(period);

  __HAL_RCC_TIM3_CLK_ENABLE();

  buzzer_tim.Instance = BUZZER_TIM_INSTANCE;
  buzzer_tim.Init.Prescaler = prescaler_reg;
  buzzer_tim.Init.CounterMode = TIM_COUNTERMODE_UP;
  buzzer_tim.Init.Period = period;
  buzzer_tim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  buzzer_tim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&buzzer_tim) != HAL_OK)
    board_error_handler();

  TIM_OC_InitTypeDef pwm_config = {0};
  pwm_config.OCMode = TIM_OCMODE_PWM1;
  pwm_config.Pulse = buzzer_waveform[0];
  pwm_config.OCPolarity = TIM_OCPOLARITY_HIGH;
  pwm_config.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&buzzer_tim, &pwm_config,
                                BUZZER_TIM_CHANNEL) != HAL_OK)
    board_error_handler();
}

static void buzzer_init(void) {
  if (buzzer_initialized)
    return;

  buzzer_gpio_init();
  buzzer_timer_init();
  buzzer_dma_init();

  buzzer_initialized = true;
}

void timer_buzzer_start(void) {
  if (buzzer_running) {
    return;
  }

  if (!buzzer_initialized) {
    buzzer_init();
  }

  if (HAL_TIM_PWM_Start(&buzzer_tim, BUZZER_TIM_CHANNEL) != HAL_OK) {
    board_error_handler();
  }
  
  if (HAL_DMA_Start_IT(&buzzer_dma, (uint32_t)buzzer_waveform,
                      (uint32_t)&buzzer_tim.Instance->CCR1,
                      BUZZER_SINE_TABLE_SIZE) != HAL_OK) {
    board_error_handler();
  }

  __HAL_TIM_ENABLE_DMA(&buzzer_tim, TIM_DMA_UPDATE);

  buzzer_running = true;
}

void timer_buzzer_stop(void) {
  if (!buzzer_running) {
    return;
  }
  __HAL_TIM_DISABLE_DMA(&buzzer_tim, TIM_DMA_UPDATE);

  if (HAL_DMA_Abort(&buzzer_dma) != HAL_OK) {
    board_error_handler();
  }

  if (HAL_TIM_PWM_Stop(&buzzer_tim, BUZZER_TIM_CHANNEL) != HAL_OK) {
    board_error_handler();
  }
  
  __HAL_TIM_SET_COMPARE(&buzzer_tim, BUZZER_TIM_CHANNEL, 0);

  buzzer_running = false;
}

void timer_init(void) { 
  buzzer_init();
}

uint32_t timer_read(void) { return HAL_GetTick(); }

void DMA1_Stream2_IRQHandler(void) { HAL_DMA_IRQHandler(&buzzer_dma); }
