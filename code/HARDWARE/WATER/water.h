#ifndef __WATER_H
#define	__WATER_H
#include "stm32f10x.h"
#include "adcx.h"
#include "delay.h"
#include "math.h"

/* ============================================================
 * 模块     : 水位传感器
 * 功能     : 水位传感器h文件
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

//#define WATER_READ_TIMES	10  //WATER传感器ADC循环读取次数

////模式选择	
////模拟AO:	1
////数字DO:	0
//#define	MODE 	1

///***************根据自己需求更改****************/
//// WATER GPIO宏定义
//#if MODE
//#define		WATER_AO_GPIO_CLK								RCC_APB2Periph_GPIOA
//#define 	WATER_AO_GPIO_PORT							GPIOA
//#define 	WATER_AO_GPIO_PIN								GPIO_Pin_1
//#define   ADC_CHANNEL               			ADC_Channel_1	// ADC 通道宏定义

//#else
//#define		WATER_DO_GPIO_CLK								RCC_APB2Periph_GPIOA
//#define 	WATER_DO_GPIO_PORT							GPIOA
//#define 	WATER_DO_GPIO_PIN								GPIO_Pin_0			

//#endif
///*********************END**********************/


//void WATER_Init(void);
//uint16_t WATER_GetData(void);


void AD_Init(void);

/**
  * 函    数：获取AD转换的值
  * 参    数：无
  * 返 回 值：AD转换的值，范围：0~4095
  */
uint16_t AD_GetValue(void);


#endif /* __WATER_H */

