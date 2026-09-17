#ifndef __HC_H
#define __HC_H
#include "sys.h"

#define HCSR04_PORT GPIOB
#define HCSR04_CLK RCC_APB2Periph_GPIOB
#define HCSR04_TRIG GPIO_Pin_0
#define HCSR04_ECHO GPIO_Pin_1
#define TRIG_Send PBout(0)
#define ECHO_Reci PBin(1)
// 定时器4设置
void hcsr04_NVIC(void);

// IO口初始化 及其他初始化
void Hcsr04Init(void);

// 打开定时器4
static void OpenTimerForHc(void);

// 关闭定时器4
static void CloseTimerForHc(void);

// 获取定时器4计数器值
u32 GetEchoTimer(void);

// 通过定时器4计数器值推算距离
float Hcsr04GetLength(void);

#endif
