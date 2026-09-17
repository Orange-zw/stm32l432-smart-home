#ifndef __SOIL_H
#define __SOIL_H
#include "stm32f10x.h"                  // Device header

#define		Soil_AO_GPIO_CLK			RCC_APB2Periph_GPIOA
#define 	Soil_AO_GPIO_PORT		    GPIOA
#define 	Soil_AO_GPIO_PIN			GPIO_Pin_4
#define     Soil_ADC_CHANNEL            ADC_Channel_4	// ADC 通道宏定义

void Soil_Init(void);
uint16_t Soil_AD_GetValue(uint8_t ADC_Channel);
uint16_t AD_GetValue(uint8_t ADC_Channel);
float Soil_Value_Conversion(void);

#endif
