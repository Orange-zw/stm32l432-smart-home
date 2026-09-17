#include "motor.h"
#include "delay.h"
#include "stm32f10x.h"       // Device header
#include "stm32f10x_gpio.h"  // 添加GPIO相关定义
#include "stm32f10x_rcc.h"   // 添加RCC时钟相关定义
#include "stm32f10x_tim.h"   // 添加定时器相关定义

// void TIM3_PWM_Init(void) {
//     GPIO_InitTypeDef GPIO_InitStruct;
//     TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
//     TIM_OCInitTypeDef TIM_OCInitStruct;

//    // 1. 使能时钟（添加AFIO时钟）
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

//    // 添加部分重映射配置
//    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);

//    // 2. 配置所有TIM3通道引脚（2025新增AFIO验证）
//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;  // PA6(TIM3_CH1), PA7(TIM3_CH2)
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStruct);

//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;  // PB0(TIM3_CH3), PB1(TIM3_CH4)
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOB, &GPIO_InitStruct);

//    // 3. 配置TIM3时基（默认生成1kHz PWM）
//    TIM_TimeBaseStruct.TIM_Prescaler = 71;              // 72MHz/(71+1)=1MHz
//    TIM_TimeBaseStruct.TIM_Period = 999;                // 1MHz/1000=1kHz
//    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
//    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
//    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStruct);

//    // 4. 配置所有通道的PWM模式
//    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
//    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
//    TIM_OCInitStruct.TIM_Pulse = 0;                     // 初始占空比0%
//    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;

//    TIM_OC1Init(TIM3, &TIM_OCInitStruct);  // CH1
//    TIM_OC2Init(TIM3, &TIM_OCInitStruct);  // CH2
//    TIM_OC3Init(TIM3, &TIM_OCInitStruct);  // CH3
//    TIM_OC4Init(TIM3, &TIM_OCInitStruct);  // CH4

//    // 5. 启用预装载寄存器
//    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
//    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
//    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
//    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);

//    TIM_ARRPreloadConfig(TIM3, ENABLE);
//    TIM_Cmd(TIM3, ENABLE);
//}

void TIM3_PWM_Init(uint16_t prescaler, uint16_t period)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	//	GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM3, ENABLE);
	//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;  // TIM3_CH1
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM_InternalClockConfig(TIM3);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	// TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;    // ARR
	// TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1; // PSC
	TIM_TimeBaseInitStructure.TIM_Period = period - 1;        // ARR
	TIM_TimeBaseInitStructure.TIM_Prescaler = prescaler - 1;  // PSC
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;  // CCR

	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	// TIM_OC2Init(TIM3, &TIM_OCInitStructure);
	// TIM_OC3Init(TIM3, &TIM_OCInitStructure);

	TIM_Cmd(TIM3, ENABLE);
}

void Set_TIM3_PWM_Duty(uint8_t channel, uint16_t duty)
{
	switch (channel)
	{
	case 1:
		TIM3->CCR1 = duty;
		break;
	case 2:
		TIM3->CCR2 = duty;
		break;
	case 3:
		TIM3->CCR3 = duty;
		break;  // PB0 对应 CH3
	case 4:
		TIM3->CCR4 = duty;
		break;
	}
}

void chuanfen(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // 开启GPIOA的时钟
	                                                       // 使用各个外设前必须开启时钟，否则对外设的操作无效

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;  // 定义结构体变量

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   // GPIO模式，赋值为推挽输出模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;          // GPIO引脚，赋值为第0号引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // GPIO速度，赋值为50MHz

	GPIO_Init(GPIOB, &GPIO_InitStructure);  // 将赋值后的构体变量传递给GPIO_Init函数
	                                        // 函数内部会自动根据结构体的参数配置相应寄存器
	                                        // 实现GPIOA的初始化
	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}

static uint8_t current_angle = 0;
void Servo_turn(u8 angle)
{
	uint16_t pulse_width = (uint16_t)(500 + (angle * 2000) / 180);
	Set_TIM3_PWM_Duty(1, pulse_width);
	current_angle = angle;
}

u8 Servo_getAngle(void)
{
	return current_angle;
}