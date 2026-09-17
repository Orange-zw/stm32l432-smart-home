#include "encoder.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "math.h"

static volatile int32_t rpm = 0;                  // ????(RPM)
static volatile uint16_t last_encoder_count = 0;  // ?????????????????
static volatile float radius = R;                 //???????
static volatile float run_girth = 2 * 3.1416 * R; //????????
float run_speed = 0;                              //????????
// ???????????
static void TIM2_Encoder_Config(void);
static void TIM1_Sampling_Config(void); // ???TIM1

/**
 * @brief  ???????????????
 * @param  ??
 * @retval ??
 */
void Encoder_Init(void)
{
  // 1. GPIO?????
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // 2. ?????????
  TIM2_Encoder_Config();  // ???????????
  TIM1_Sampling_Config(); // ????????????TIM1
}

/**
 * @brief  ????????????
 * @param  ??
 * @retval ????(RPM)
 */
int32_t Encoder_GetRPM(void)
{
  return rpm;
}

//??????
// cm/s
float Encoder_GetSpeed(void)
{
  return run_speed;
}

/**
 * @brief  ????????????????
 * @param  ??
 * @retval ???????????
 */
int32_t Encoder_GetCount(void)
{
  return (int16_t)TIM_GetCounter(TIM2);
}

void Encoder_setRadius(float r)
{
  // do nothing
  radius = r;
  run_girth = 2 * 3.1416 * radius;
}

/**
 * @brief  ???????????????
 * @param  ??
 * @retval ??
 */
void Encoder_ResetCount(void)
{
  TIM_SetCounter(TIM2, 0);
  last_encoder_count = 0;
}

/**
 * @brief  TIM1??????????? (???TIM1)
 * @param  ??
 * @retval ??
 */
void TIM1_UP_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
  {
    uint16_t current_count = TIM_GetCounter(TIM2);
    // ????????????????
    int16_t delta = (int16_t)(current_count - last_encoder_count);
    float rpm_tmp = (delta / (2 * ENCODER_PPR * (SAMPLE_TIME_MS / 1000.0))) * 60;
    rpm = (int32_t)fabsf(rpm_tmp);
    run_speed = (float)rpm * (float)run_girth * 60 / 100000; //  /100/1000
    last_encoder_count = current_count;
    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
  }
}

/**
 * @brief  TIM2??????????
 * @param  ??
 * @retval ??
 */
static void TIM2_Encoder_Config(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_ICInitTypeDef TIM_ICInitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12,
                             TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);

  TIM_ICInitStructure.TIM_ICFilter = 0x0;
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInit(TIM2, &TIM_ICInitStructure);

  TIM_ICInitStructure.TIM_Channel = TIM_Channel_3;
  TIM_ICInit(TIM2, &TIM_ICInitStructure);

  TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief  TIM1????????????? (???TIM1)
 * @param  ??
 * @retval ??
 */
static void TIM1_Sampling_Config(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); // TIM1??APB2??????

  // 72MHz/7200 = 10kHz, 10kHz/100 = 100Hz (10ms)
  TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;
  TIM_TimeBaseStructure.TIM_Period = (SAMPLE_TIME_MS * 10) - 1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

  TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
  TIM_Cmd(TIM1, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn; // TIM1????????
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
