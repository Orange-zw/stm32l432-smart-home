#ifndef __TS_H
#define __TS_H
#include "stm32f10x.h"
#include "adc.h"
#include "delay.h"
#include "math.h"

/* ============================================================
 * 模块     : TS-300B浊度传感器
 * 功能     : TS-300B浊度传感器h文件
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#define TS_READ_TIMES 10 // TS浊度传感器ADC循环读取次数
#define TS_K 2047.19

/***************根据自己需求更改****************/
// TS GPIO宏定义

#define TS_GPIO_CLK RCC_APB2Periph_GPIOA
#define TS_GPIO_PORT GPIOA
#define TS_GPIO_PIN GPIO_Pin_1
#define ADC_CHANNEL ADC_Channel_1 // ADC 通道宏定义

/*********************END**********************/

void TS_Init(void);
float TS_GetData(void);

#endif /* __TS_H */
