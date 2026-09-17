#ifndef __YFS401_H
#define __YFS401_H
#include "stm32f10x.h"

// 用户可在此处修改脉冲输入引脚及外部中断线映射
// 默认使用 PA0 -> EXTI0 下降沿计数
#define YFS401_GPIO_CLOCK      RCC_APB2Periph_GPIOA
#define YFS401_GPIO_PORT       GPIOA
#define YFS401_GPIO_PIN        GPIO_Pin_7
#define YFS401_EXTI_LINE       EXTI_Line7
#define YFS401_GPIO_PORTSOURCE GPIO_PortSourceGPIOA
#define YFS401_GPIO_PINSOURCE  GPIO_PinSource7
#define YFS401_EXTI_IRQn       EXTI9_5_IRQn

// YF-S401 参数：F(Hz) = 98 * Q(L/min)
// => 每升脉冲常数 k = 60 * 98 = 5880 pulses/L
#define YFS401_PULSES_PER_LITER  (5400UL)

void YFS401_Init(void);
float YFS401_GetTotalLiters(void);
float YFS401_GetFlowLMin(void);
uint32_t YFS401_GetPulseCount(void);
void YFS401_ResetTotal(void);

#endif
