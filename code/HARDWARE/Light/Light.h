#ifndef __LIGHT_H
#define __LIGHT_H
#include "stm32f10x.h"                  // Device header

#define		Light_AO_GPIO_CLK			RCC_APB2Periph_GPIOA
#define 	Light_AO_GPIO_PORT		    GPIOA
#define 	Light_AO_GPIO_PIN			GPIO_Pin_5
#define     Light_ADC_CHANNEL           ADC_Channel_5	// ADC 通道宏定义

void Light_Init(void);
uint16_t Light_AD_GetValue(uint8_t ADC_Channel);
uint16_t AD_GetValue(uint8_t ADC_Channel);
float Light_Value_Conversion(void);

#endif
