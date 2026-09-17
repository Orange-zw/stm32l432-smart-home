#include "stm32f10x.h"
#include "YFS401.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"
#include "misc.h"


#ifndef TIM2_IRQn
#define TIM2_IRQn ((IRQn_Type)28)
#endif

static volatile uint32_t s_yfs401_pulse_count = 0;
static volatile uint32_t s_yfs401_last_sec_count = 0;
static volatile float s_yfs401_flow_l_min = 0.0f;

static void YFS401_TIM2_1Hz_Config(void)
{
	RCC_ClocksTypeDef clocks;
	RCC_GetClocksFreq(&clocks);
	/* TIM2 时钟：当 PCLK1 分频不为1 时，定时器时钟为 PCLK1*2 */
	uint32_t pclk1 = clocks.PCLK1_Frequency;
	uint32_t timclk = (RCC->CFGR & RCC_CFGR_PPRE1_2) ? (pclk1 * 2U) : pclk1;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	/* 目标：1Hz 更新事件。选择 10kHz 计数频率，Period=10000-1 */
	uint16_t prescaler = (uint16_t)((timclk / 10000U) - 1U);
	uint16_t period = 10000U - 1U;

	TIM_TimeBaseInitTypeDef tim;
	tim.TIM_ClockDivision = TIM_CKD_DIV1;
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Period = period;
	tim.TIM_Prescaler = prescaler;
	tim.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &tim);

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = TIM2_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 1;
	nvic.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&nvic);

	TIM_Cmd(TIM2, ENABLE);
}

void YFS401_Init(void)
{
	RCC_APB2PeriphClockCmd(YFS401_GPIO_CLOCK | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitTypeDef gpio;
	gpio.GPIO_Pin = YFS401_GPIO_PIN;
	gpio.GPIO_Mode = GPIO_Mode_IPU; /* 上拉输入，适配开漏/集电极输出 */
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(YFS401_GPIO_PORT, &gpio);

	GPIO_EXTILineConfig(YFS401_GPIO_PORTSOURCE, YFS401_GPIO_PINSOURCE);

	EXTI_InitTypeDef exti;
	EXTI_StructInit(&exti);
	exti.EXTI_Line = YFS401_EXTI_LINE;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_Trigger = EXTI_Trigger_Falling; /* 下降沿统计 */
	exti.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti);

	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = YFS401_EXTI_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic);

	YFS401_TIM2_1Hz_Config();
}

uint32_t YFS401_GetPulseCount(void)
{
	return s_yfs401_pulse_count;
}

float YFS401_GetTotalLiters(void)
{
	return (float)s_yfs401_pulse_count / (float)YFS401_PULSES_PER_LITER;
}

float YFS401_GetFlowLMin(void)
{
	return s_yfs401_flow_l_min;
}

void YFS401_ResetTotal(void)
{
	__disable_irq();
	s_yfs401_pulse_count = 0;
	s_yfs401_last_sec_count = 0;
	s_yfs401_flow_l_min = 0.0f;
	__enable_irq();
}

/* EXTI 中断：计数脉冲 */
void EXTI9_5_IRQHandler(void)
{
	if (EXTI_GetITStatus(YFS401_EXTI_LINE) == SET)
	{
		/* 简单确认电平为低，抑制抖动（可根据硬件适当调整/去掉） */
		if (GPIO_ReadInputDataBit(YFS401_GPIO_PORT, YFS401_GPIO_PIN) == Bit_RESET)
		{
			s_yfs401_pulse_count++;
		}
		EXTI_ClearITPendingBit(YFS401_EXTI_LINE);
	}
}

/* TIM2 1Hz：计算 1s 内脉冲并换算 L/min */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		uint32_t now = s_yfs401_pulse_count;
		uint32_t delta = now - s_yfs401_last_sec_count;
		s_yfs401_last_sec_count = now;
		s_yfs401_flow_l_min = ((float)delta / (float)YFS401_PULSES_PER_LITER) * 60.0f;
	}
}
