/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "hardware/rgb_api.h"

#if defined(RGB_ENABLE)

#include "hardware/hardware.h"
#include "rgb.h"
#include "stm32f7xx_hal.h"

#if !defined(RGB_BACKEND_WS2812_TIM2_CH1)
#error "Unsupported RGB backend for STM32F723"
#endif
#define WS2812_BITS_PER_LED 24u
/* Refill eight LEDs per half-buffer. This lowers the worst-case interrupt rate
 * from 33.3 kHz (one callback per LED) to 4.17 kHz while keeping the buffer
 * small and every refill deadline a comfortable 240 us. */
#define WS2812_LEDS_PER_HALF 8u
#define WS2812_HALF_WORDS (WS2812_BITS_PER_LED * WS2812_LEDS_PER_HALF)
#define WS2812_DMA_WORDS (WS2812_HALF_WORDS * 2u)
#define WS2812_PERIOD 134u
#define WS2812_ZERO 43u
#define WS2812_ONE 86u

static TIM_HandleTypeDef tim_handle;
static DMA_HandleTypeDef dma_handle;
__attribute__((aligned(32))) static uint32_t dma_buffer[WS2812_DMA_WORDS];
static uint8_t tx_frame[RGB_FRAME_BYTES];
static volatile bool tx_busy;
static volatile bool tx_stop_pending;
static uint16_t tx_next_led;
static bool half_contains_data[2];

static void rgb_dma_clean(uint32_t *first, uint32_t words) {
  if ((SCB->CCR & SCB_CCR_DC_Msk) != 0u)
    SCB_CleanDCache_by_Addr(first,
                           (int32_t)(words * (uint32_t)sizeof(uint32_t)));
}

static void encode_byte(uint8_t value, uint32_t **dst) {
  for (uint8_t mask = 0x80u; mask != 0u; mask >>= 1u) {
    **dst = (value & mask) != 0u ? WS2812_ONE : WS2812_ZERO;
    (*dst)++;
  }
}

static void fill_half(uint8_t half) {
  uint32_t *dst = dma_buffer + (uint32_t)half * WS2812_HALF_WORDS;
  bool contains_data = false;

  memset(dst, 0, WS2812_HALF_WORDS * sizeof(uint32_t));
  for (uint8_t slot = 0u;
       slot < WS2812_LEDS_PER_HALF && tx_next_led < RGB_LED_COUNT; slot++) {
    const uint8_t *pixel = tx_frame + tx_next_led * RGB_BYTES_PER_PIXEL;
    /* WS2812 is GRB on the wire; the bridge remains logical RGB. */
    encode_byte(pixel[1], &dst);
    encode_byte(pixel[0], &dst);
    encode_byte(pixel[2], &dst);
    tx_next_led++;
    contains_data = true;
  }
  half_contains_data[half] = contains_data;
  rgb_dma_clean(dma_buffer + (uint32_t)half * WS2812_HALF_WORDS,
                WS2812_HALF_WORDS);
}

static void half_completed(uint8_t half) {
  if (!tx_busy || tx_stop_pending)
    return;

  if (!half_contains_data[half]) {
    /* A completely-low half lasts 240 us, already exceeding the WS2812 latch
     * requirement. Leave both circular halves low and defer HAL shutdown to
     * the main loop: HAL_TIM_PWM_Stop_DMA() is not safe from inside its own
     * completion callback because the DMA handle can remain in ABORT state. */
    tx_stop_pending = true;
    return;
  }
  fill_half(half);
}

bool rgb_backend_init(void) {
  GPIO_InitTypeDef gpio = {0};
  TIM_OC_InitTypeDef output = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();

  gpio.Pin = RGB_DATA_GPIO_PIN;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(RGB_DATA_GPIO_PORT, &gpio);

  dma_handle.Instance = DMA1_Stream5;
  dma_handle.Init.Channel = DMA_CHANNEL_3;
  dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
  dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
  dma_handle.Init.MemInc = DMA_MINC_ENABLE;
  dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  dma_handle.Init.Mode = DMA_CIRCULAR;
  dma_handle.Init.Priority = DMA_PRIORITY_HIGH;
  dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&dma_handle) != HAL_OK)
    return false;

  tim_handle.Instance = TIM2;
  tim_handle.Init.Prescaler = 0;
  tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  tim_handle.Init.Period = WS2812_PERIOD;
  tim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  tim_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&tim_handle) != HAL_OK)
    return false;

  output.OCMode = TIM_OCMODE_PWM1;
  output.Pulse = 0;
  output.OCPolarity = TIM_OCPOLARITY_HIGH;
  output.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&tim_handle, &output, TIM_CHANNEL_1) != HAL_OK)
    return false;

  __HAL_LINKDMA(&tim_handle, hdma[TIM_DMA_ID_CC1], dma_handle);
  memset(dma_buffer, 0, sizeof(dma_buffer));
  rgb_dma_clean(dma_buffer, WS2812_DMA_WORDS);

  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  return true;
}

bool rgb_backend_submit(const uint8_t *rgb, uint16_t led_count) {
  if (rgb == NULL || led_count != RGB_LED_COUNT || tx_busy)
    return false;

  memcpy(tx_frame, rgb, RGB_FRAME_BYTES);
  tx_next_led = 0;
  tx_stop_pending = false;
  fill_half(0);
  fill_half(1);
  tx_busy = true;

  if (HAL_TIM_PWM_Start_DMA(&tim_handle, TIM_CHANNEL_1, dma_buffer,
                            WS2812_DMA_WORDS) != HAL_OK) {
    tx_busy = false;
    return false;
  }
  return true;
}

bool rgb_backend_is_busy(void) { return tx_busy; }

void rgb_backend_task(void) {
  if (!tx_stop_pending)
    return;

  /* Run the synchronous abort outside the DMA callback. The wire is already
   * held low by both halves, so the few cycles spent disabling the peripheral
   * cannot corrupt the latched LED frame. */
  __HAL_TIM_DISABLE_DMA(&tim_handle, TIM_DMA_CC1);
  if (HAL_DMA_Abort(&dma_handle) != HAL_OK ||
      HAL_TIM_PWM_Stop(&tim_handle, TIM_CHANNEL_1) != HAL_OK)
    board_error_handler();

  tx_busy = false;
  tx_stop_pending = false;
}

void DMA1_Stream5_IRQHandler(void) { HAL_DMA_IRQHandler(&dma_handle); }

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
  if (htim == &tim_handle)
    half_completed(0);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
  if (htim == &tim_handle)
    half_completed(1);
}

#endif
