#ifndef _ADC_H
#define _ADC_H

#include "stm32f10x.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "delay.h"

void ADC_Config(void);
void Get_ADC_Value(void);
void Adc_Init(u8 pa_index);
u16 Get_Adc(u8 ch);
u16 Get_Adc_Average(u8 ch, u8 times);

void ADC_DMA_Init(u8 pa_index);
u16 Get_Adc_DMA(u8 ch);
u16 Get_Adc_DMA_Average(u8 ch, u8 times);
#endif
