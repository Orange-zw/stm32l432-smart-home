#ifndef __FSR402_H
#define	__FSR402_H
#include "stm32f10x.h"
#include "adcx.h"
#include "delay.h"
#include "math.h"

/* ============================================================
 * 模块     : FSR402薄膜压力传感器
 * 功能     : FSR402薄膜压力传感器h文件
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#define FSR402_READ_TIMES	10  //薄膜压力传感器ADC循环读取次数

//模式选择	
//模拟AO:	1
//数字DO:	0
#define	MODE 	1

/***************根据自己需求更改****************/
// FSR402 GPIO宏定义
#if MODE
#define		FSR402_AO_GPIO_CLK								RCC_APB2Periph_GPIOA
#define 	FSR402_AO_GPIO_PORT								GPIOA
#define 	FSR402_AO_GPIO_PIN								GPIO_Pin_0
#define   ADC_CHANNEL               				ADC_Channel_0	// ADC 通道宏定义

#else
#define		FSR402_DO_GPIO_CLK								RCC_APB2Periph_GPIOA
#define 	FSR402_DO_GPIO_PORT								GPIOA
#define 	FSR402_DO_GPIO_PIN								GPIO_Pin_0			

#endif
/*********************END**********************/


void FSR402_Init(void);
uint16_t FSR402_GetData(void);

#endif /* __ADC_H */

