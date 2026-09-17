/* ============================================================
 * 模块     : 定时器驱动
 * 功能     : TIM2 配置为 1 ms 周期中断，作为系统心跳
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "stm32f10x.h" // Device header
#include "timer.h"	   // Device header
void Timer_Init(uint16_t prescaler, uint16_t period)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_InternalClockConfig(TIM2);

	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = period - 1; // 定时一秒
	TIM_TimeBaseInitStructure.TIM_Prescaler = prescaler - 1;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM2, ENABLE);

	while (TIM_GetCounter(TIM2) == 0)
		; // 等待计数器开始计数
		  // 如果能执行到这里,说明定时器已启动
}

// extern char TIMER_IT;
// void TIM2_IRQHandler(void)
//{
//	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
//	{
//		TIMER_IT = 1;
//		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
//	}
// }
