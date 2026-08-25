/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "hardware/hardware.h"

#include "stm32f7xx_hal.h"

static GPIO_TypeDef *channel_ports[] = {
    GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA,
    GPIOB, GPIOB, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC,
};
static const uint16_t channel_pins[] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5,
    GPIO_PIN_6, GPIO_PIN_7, GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_0, GPIO_PIN_1,
    GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5,
};

_Static_assert(M_ARRAY_SIZE(channel_ports) == ADC_NUM_CHANNELS,
               "Invalid ADC channel port table");
_Static_assert(M_ARRAY_SIZE(channel_pins) == ADC_NUM_CHANNELS,
               "Invalid ADC channel pin table");

#if ADC_NUM_MUX_INPUTS > 0
static const uint32_t mux_input_channels[] = ADC_MUX_INPUT_CHANNELS;
static GPIO_TypeDef *mux_select_ports[] = ADC_MUX_SELECT_PORTS;
static const uint16_t mux_select_pins[] = ADC_MUX_SELECT_PINS;
static const uint16_t mux_input_matrix[][ADC_NUM_MUX_INPUTS] =
    ADC_MUX_INPUT_MATRIX;

_Static_assert(M_ARRAY_SIZE(mux_input_channels) == ADC_NUM_MUX_INPUTS,
               "Invalid ADC mux input table");
_Static_assert(M_ARRAY_SIZE(mux_select_ports) == ADC_NUM_MUX_SELECT_PINS,
               "Invalid mux select port table");
_Static_assert(M_ARRAY_SIZE(mux_select_pins) == ADC_NUM_MUX_SELECT_PINS,
               "Invalid mux select pin table");
_Static_assert(M_ARRAY_SIZE(mux_input_matrix) ==
                   (1u << ADC_NUM_MUX_SELECT_PINS),
               "Invalid ADC mux matrix");
#endif

#if ADC_NUM_RAW_INPUTS > 0
static const uint32_t raw_input_channels[] = ADC_RAW_INPUT_CHANNELS;
static const uint16_t raw_input_vector[] = ADC_RAW_INPUT_VECTOR;

_Static_assert(M_ARRAY_SIZE(raw_input_channels) == ADC_NUM_RAW_INPUTS,
               "Invalid raw ADC input table");
_Static_assert(M_ARRAY_SIZE(raw_input_vector) == ADC_NUM_RAW_INPUTS,
               "Invalid raw ADC mapping table");
#endif

static ADC_HandleTypeDef adc_handle;
static DMA_HandleTypeDef dma_handle;
#if ADC_NUM_MUX_INPUTS > 0
static TIM_HandleTypeDef tim_handle;
#endif

static volatile bool adc_initialized;
#define ADC_DMA_SAMPLES                                                     \
  (M_DIV_CEIL(ADC_NUM_MUX_INPUTS + ADC_NUM_RAW_INPUTS, 16u) * 16u)
__attribute__((aligned(32))) static volatile uint16_t
    adc_buffer[ADC_DMA_SAMPLES];
static volatile uint16_t adc_values[NUM_KEYS];

static void adc_buffer_invalidate(void) {
  if ((SCB->CCR & SCB_CCR_DC_Msk) != 0u) {
    const uintptr_t first = (uintptr_t)adc_buffer & ~(uintptr_t)31u;
    const uintptr_t last =
        ((uintptr_t)adc_buffer + sizeof(adc_buffer) + 31u) & ~(uintptr_t)31u;
    SCB_InvalidateDCache_by_Addr((uint32_t *)first, (int32_t)(last - first));
  }
}

void analog_init(void) {
  ADC_ChannelConfTypeDef channel = {0};

  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
#if ADC_NUM_MUX_INPUTS > 0
  __HAL_RCC_TIM10_CLK_ENABLE();
#endif

  adc_handle.Instance = ADC1;
  adc_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  adc_handle.Init.Resolution = ADC_RESOLUTION_HAL;
  adc_handle.Init.ScanConvMode = ADC_SCAN_ENABLE;
  adc_handle.Init.ContinuousConvMode = DISABLE;
  adc_handle.Init.DiscontinuousConvMode = DISABLE;
  adc_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  adc_handle.Init.NbrOfConversion = ADC_NUM_MUX_INPUTS + ADC_NUM_RAW_INPUTS;
  adc_handle.Init.DMAContinuousRequests = DISABLE;
  adc_handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&adc_handle) != HAL_OK)
    board_error_handler();

#if ADC_NUM_MUX_INPUTS > 0
  for (uint32_t i = 0; i < ADC_NUM_MUX_INPUTS; i++) {
    GPIO_InitTypeDef gpio = {0};
    channel.Channel = mux_input_channels[i];
    channel.Rank = i + 1u;
    channel.SamplingTime = ADC_NUM_SAMPLE_CYCLES;
    if (HAL_ADC_ConfigChannel(&adc_handle, &channel) != HAL_OK)
      board_error_handler();

    gpio.Pin = channel_pins[mux_input_channels[i]];
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(channel_ports[mux_input_channels[i]], &gpio);
  }

  for (uint32_t i = 0; i < ADC_NUM_MUX_SELECT_PINS; i++) {
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = mux_select_pins[i];
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(mux_select_ports[i], &gpio);
    HAL_GPIO_WritePin(mux_select_ports[i], mux_select_pins[i], GPIO_PIN_RESET);
  }
#endif

#if ADC_NUM_RAW_INPUTS > 0
  for (uint32_t i = 0; i < ADC_NUM_RAW_INPUTS; i++) {
    GPIO_InitTypeDef gpio = {0};
    channel.Channel = raw_input_channels[i];
    channel.Rank = ADC_NUM_MUX_INPUTS + i + 1u;
    channel.SamplingTime = ADC_NUM_SAMPLE_CYCLES;
    if (HAL_ADC_ConfigChannel(&adc_handle, &channel) != HAL_OK)
      board_error_handler();

    gpio.Pin = channel_pins[raw_input_channels[i]];
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(channel_ports[raw_input_channels[i]], &gpio);
  }
#endif

  dma_handle.Instance = DMA2_Stream0;
  dma_handle.Init.Channel = DMA_CHANNEL_0;
  dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
  dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
  dma_handle.Init.MemInc = DMA_MINC_ENABLE;
  dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  dma_handle.Init.Mode = DMA_CIRCULAR;
  dma_handle.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&dma_handle) != HAL_OK)
    board_error_handler();
  __HAL_LINKDMA(&adc_handle, DMA_Handle, dma_handle);

#if ADC_NUM_MUX_INPUTS > 0
  tim_handle.Instance = TIM10;
  tim_handle.Init.Prescaler = 0;
  tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  tim_handle.Init.Period =
      (F_CPU / 1000000u) * (uint32_t)ADC_SAMPLE_DELAY - 1u;
  tim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  tim_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&tim_handle) != HAL_OK)
    board_error_handler();
#endif

  HAL_NVIC_SetPriority(ADC_IRQn, 1, 0);
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(ADC_IRQn);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
#if ADC_NUM_MUX_INPUTS > 0
  HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
#endif

  adc_buffer_invalidate();
  if (HAL_ADC_Start_DMA(&adc_handle, (uint32_t *)adc_buffer,
                        ADC_NUM_MUX_INPUTS + ADC_NUM_RAW_INPUTS) != HAL_OK)
    board_error_handler();
  while (!adc_initialized)
    ;
}

void analog_task(void) {}

uint16_t analog_read(uint8_t key) { return adc_values[key]; }

void ADC_IRQHandler(void) { HAL_ADC_IRQHandler(&adc_handle); }
void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&dma_handle); }
#if ADC_NUM_MUX_INPUTS > 0
void TIM1_UP_TIM10_IRQHandler(void) { HAL_TIM_IRQHandler(&tim_handle); }
#endif

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
#if ADC_NUM_MUX_INPUTS > 0
  static uint8_t mux_channel;
#endif
  if (hadc != &adc_handle)
    return;

  adc_buffer_invalidate();
#if ADC_NUM_MUX_INPUTS > 0
  for (uint32_t i = 0; i < ADC_NUM_MUX_INPUTS; i++) {
    const uint16_t key = mux_input_matrix[mux_channel][i];
    if (key != 0u)
      adc_values[key - 1u] = adc_buffer[i];
  }
#endif
#if ADC_NUM_RAW_INPUTS > 0
  for (uint32_t i = 0; i < ADC_NUM_RAW_INPUTS; i++) {
    const uint16_t key = raw_input_vector[i];
    if (key != 0u)
      adc_values[key - 1u] = adc_buffer[ADC_NUM_MUX_INPUTS + i];
  }
#endif

#if ADC_NUM_MUX_INPUTS > 0
  mux_channel = (uint8_t)((mux_channel + 1u) &
                          ((1u << ADC_NUM_MUX_SELECT_PINS) - 1u));
  adc_initialized |= mux_channel == 0u;
  for (uint32_t i = 0; i < ADC_NUM_MUX_SELECT_PINS; i++) {
    HAL_GPIO_WritePin(mux_select_ports[i], mux_select_pins[i],
                      ((((uint32_t)mux_channel) >> i) & 1u) != 0u
                          ? GPIO_PIN_SET
                          : GPIO_PIN_RESET);
  }
  if (HAL_TIM_Base_Start_IT(&tim_handle) != HAL_OK)
    board_error_handler();
#else
  adc_initialized = true;
  adc_buffer_invalidate();
  if (HAL_ADC_Start_DMA(&adc_handle, (uint32_t *)adc_buffer,
                        ADC_NUM_MUX_INPUTS + ADC_NUM_RAW_INPUTS) != HAL_OK)
    board_error_handler();
#endif
}

#if ADC_NUM_MUX_INPUTS > 0
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim != &tim_handle)
    return;
  if (HAL_TIM_Base_Stop_IT(&tim_handle) != HAL_OK)
    board_error_handler();
  adc_buffer_invalidate();
  if (HAL_ADC_Start_DMA(&adc_handle, (uint32_t *)adc_buffer,
                        ADC_NUM_MUX_INPUTS + ADC_NUM_RAW_INPUTS) != HAL_OK)
    board_error_handler();
}
#endif
