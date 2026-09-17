#ifndef __HW_H
#define	__HW_H
#include "stm32f10x.h"
#include "adcx.h"
#include "delay.h"
#include "math.h"

/* ============================================================
 * 模块     : 光电红外传感器
 * 功能     : 光电红外传感器h文件
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */


/***************根据自己需求更改****************/
// HW GPIO宏定义

#define		HW_GPIO_CLK								RCC_APB2Periph_GPIOB
#define 	HW_GPIO_PORT							GPIOB
#define 	HW_GPIO_PIN								GPIO_Pin_12			

/*********************END**********************/


void HW_Init(void);
uint16_t HW_GetData(void);

#endif /* __ADC_H */

