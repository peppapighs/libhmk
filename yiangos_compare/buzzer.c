#include "Buzzer.h"

#include "Timers.h"
#include "assert.h"

/**
 * @brief python -> for i in range(180):
 *  print(int(math.sin(math.radians(i*2))), end=",")
 *
 */
static const float m_sin[180] = {
  0.0,
  0.03489949670250097,
  0.0697564737441253,
  0.10452846326765347,
  0.13917310096006544,
  0.17364817766693033,
  0.20791169081775934,
  0.24192189559966773,
  0.27563735581699916,
  0.3090169943749474,
  0.3420201433256687,
  0.374606593415912,
  0.4067366430758002,
  0.4383711467890774,
  0.4694715627858908,
  0.49999999999999994,
  0.5299192642332049,
  0.5591929034707469,
  0.5877852522924731,
  0.6156614753256583,
  0.6427876096865393,
  0.6691306063588582,
  0.6946583704589973,
  0.7193398003386512,
  0.7431448254773942,
  0.766044443118978,
  0.788010753606722,
  0.8090169943749475,
  0.8290375725550417,
  0.8480480961564261,
  0.8660254037844386,
  0.8829475928589269,
  0.898794046299167,
  0.9135454576426009,
  0.9271838545667874,
  0.9396926207859083,
  0.9510565162951535,
  0.9612616959383189,
  0.9702957262759965,
  0.9781476007338056,
  0.984807753012208,
  0.9902680687415704,
  0.9945218953682733,
  0.9975640502598242,
  0.9993908270190958,
  1.0,
  0.9993908270190958,
  0.9975640502598242,
  0.9945218953682733,
  0.9902680687415704,
  0.984807753012208,
  0.9781476007338057,
  0.9702957262759965,
  0.9612616959383189,
  0.9510565162951536,
  0.9396926207859084,
  0.9271838545667874,
  0.9135454576426009,
  0.8987940462991669,
  0.8829475928589269,
  0.8660254037844387,
  0.8480480961564261,
  0.8290375725550417,
  0.8090169943749475,
  0.788010753606722,
  0.766044443118978,
  0.7431448254773942,
  0.7193398003386511,
  0.6946583704589971,
  0.6691306063588583,
  0.6427876096865395,
  0.6156614753256584,
  0.5877852522924732,
  0.5591929034707469,
  0.5299192642332049,
  0.49999999999999994,
  0.4694715627858907,
  0.4383711467890773,
  0.40673664307580043,
  0.37460659341591224,
  0.3420201433256689,
  0.3090169943749475,
  0.2756373558169992,
  0.24192189559966773,
  0.20791169081775931,
  0.17364817766693028,
  0.13917310096006533,
  0.10452846326765373,
  0.06975647374412552,
  0.03489949670250114,
  1.2246467991473532e-16,
  -0.0348994967025009,
  -0.06975647374412527,
  -0.1045284632676535,
  -0.13917310096006552,
  -0.17364817766693047,
  -0.2079116908177595,
  -0.2419218955996675,
  -0.275637355816999,
  -0.3090169943749473,
  -0.34202014332566866,
  -0.374606593415912,
  -0.4067366430758002,
  -0.43837114678907746,
  -0.46947156278589086,
  -0.5000000000000001,
  -0.5299192642332048,
  -0.5591929034707467,
  -0.587785252292473,
  -0.6156614753256582,
  -0.6427876096865393,
  -0.6691306063588582,
  -0.6946583704589973,
  -0.7193398003386512,
  -0.7431448254773944,
  -0.7660444431189779,
  -0.7880107536067221,
  -0.8090169943749473,
  -0.8290375725550418,
  -0.848048096156426,
  -0.8660254037844385,
  -0.882947592858927,
  -0.8987940462991668,
  -0.913545457642601,
  -0.9271838545667873,
  -0.9396926207859084,
  -0.9510565162951535,
  -0.961261695938319,
  -0.9702957262759965,
  -0.9781476007338056,
  -0.984807753012208,
  -0.9902680687415703,
  -0.9945218953682734,
  -0.9975640502598242,
  -0.9993908270190958,
  -1.0,
  -0.9993908270190958,
  -0.9975640502598243,
  -0.9945218953682734,
  -0.9902680687415704,
  -0.9848077530122081,
  -0.9781476007338056,
  -0.9702957262759966,
  -0.9612616959383188,
  -0.9510565162951536,
  -0.9396926207859083,
  -0.9271838545667874,
  -0.9135454576426011,
  -0.898794046299167,
  -0.8829475928589271,
  -0.8660254037844386,
  -0.8480480961564261,
  -0.8290375725550416,
  -0.8090169943749476,
  -0.7880107536067218,
  -0.7660444431189781,
  -0.7431448254773946,
  -0.7193398003386511,
  -0.6946583704589976,
  -0.6691306063588581,
  -0.6427876096865396,
  -0.6156614753256582,
  -0.5877852522924734,
  -0.5591929034707466,
  -0.529919264233205,
  -0.5000000000000004,
  -0.4694715627858908,
  -0.4383711467890778,
  -0.40673664307580015,
  -0.37460659341591235,
  -0.3420201433256686,
  -0.3090169943749477,
  -0.27563735581699894,
  -0.24192189559966787,
  -0.20791169081775987,
  -0.1736481776669304,
  -0.13917310096006588,
  -0.10452846326765342,
  -0.06975647374412564,
  -0.034899496702500823,
};
static uint16_t m_timerCCRBuf[180];

void BuzzerBeepMode(BuzzerBeepModes_t mode, uint32_t beepFreq, float beepVolume)
{
  switch (mode)
  {
  case eBuzzerTestBeep: BuzzerBeep(beepFreq, beepVolume, 1000); break;
  case eBuzzerBeepStart: BuzzerBeep(beepFreq, beepVolume, 200); break;
  case eBuzzerBeepControl:
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    break;
  case eBuzzerMacLayoutControl: BuzzerBeep(beepFreq, beepVolume, 1000); break;
  case eBuzzerWinLayoutControl:
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    break;
  case eBuzzerGoDfu:
    BuzzerBeep(beepFreq, beepVolume, 300);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    break;
  case eBuzzerDfuDone: BuzzerBeep(beepFreq, beepVolume, 1000); break;
  case eBuzzerSample_1:
    BuzzerBeep(beepFreq, beepVolume, 300);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    break;
  case eBuzzerSample_2:
    BuzzerBeep(beepFreq, beepVolume, 500);
    HAL_Delay(500);
    BuzzerBeep(beepFreq, beepVolume, 500);
    break;
  case eBuzzerSample_3:
    BuzzerBeep(beepFreq, beepVolume, 300);
    HAL_Delay(300);
    BuzzerBeep(beepFreq, beepVolume, 300);
    break;
  case eBuzzerSample_4:
    BuzzerBeep(beepFreq, beepVolume, 800);
    HAL_Delay(800);
    BuzzerBeep(beepFreq, beepVolume, 800);
    break;
  case eBuzzerSample_5:
    BuzzerBeep(beepFreq, beepVolume, 200);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 500);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 200);
    break;
  case eBuzzerSample_6:
    BuzzerBeep(beepFreq, beepVolume, 200);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 200);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 200);
    break;
  case eBuzzerSample_7:
    BuzzerBeep(beepFreq, beepVolume, 200);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 200);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 500);
    break;
  case eBuzzerSample_8:
    BuzzerBeep(beepFreq, beepVolume, 500);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 200);
    HAL_Delay(200);
    BuzzerBeep(beepFreq, beepVolume, 500);
    break;
  case eBuzzerSample_9:
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    HAL_Delay(100);
    BuzzerBeep(beepFreq, beepVolume, 100);
    break;
  }
}

void BuzzerBeep(uint32_t freq, float volume, uint32_t duration_ms)
{
  static const uint32_t TIMER_CLK = 72000000;
  assert(volume < 1.f);
  /**
   * @brief To output a sine with a frequency of 1hz, we must have a PWM with a
   * frequency equal to the size of the m_sin array.
   */
  uint32_t pwm_freq = freq * sizeof(m_sin) / sizeof(*m_sin);
  assert(TIMER_CLK / pwm_freq < UINT16_MAX);

  /**
   * @brief Calculating the ARR value for the timer, the maximum ARR value will
   * be the maximum volume in CCR register.
   */
  uint16_t tim_arr_val = TIMER_CLK / pwm_freq;

  for (size_t i = 0; i < sizeof(m_sin) / sizeof(*m_sin); ++i)
  {
    /**
     * @brief Shifting the sine to the positive part. And adjusting its values
     * ​​to the required volume.
     */
    m_timerCCRBuf[i] = (m_sin[i] + 1) * (tim_arr_val >> 1) * volume;
  }

  TimersGetHandleTim3()->Instance->PSC = 0;
  TimersGetHandleTim3()->Instance->ARR = tim_arr_val - 1;

  TimersGetHandleTim4()->Instance->PSC = 0;
  TimersGetHandleTim4()->Instance->ARR = tim_arr_val - 1;

  HAL_TIM_PWM_Start(TimersGetHandleTim3(), TIM_CHANNEL_1);
  HAL_DMA_Start(TimersGetHandleTim4()->hdma[TIM_DMA_ID_UPDATE],
                (uint32_t)m_timerCCRBuf,
                (uint32_t)&TimersGetHandleTim3()->Instance->CCR1,
                sizeof(m_timerCCRBuf) / sizeof(*m_timerCCRBuf));
  __HAL_TIM_ENABLE_DMA(TimersGetHandleTim4(), TIM_DMA_UPDATE);
  HAL_TIM_Base_Start(TimersGetHandleTim4());

  HAL_Delay(duration_ms);

  HAL_TIM_PWM_Stop(TimersGetHandleTim3(), TIM_CHANNEL_1);
  __HAL_TIM_DISABLE_DMA(TimersGetHandleTim4(), TIM_DMA_UPDATE);
  HAL_TIM_Base_Stop(TimersGetHandleTim4());
}