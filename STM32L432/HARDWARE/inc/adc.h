#ifndef __ADC_H
#define __ADC_H
#include "sys.h"
#include "delay.h"
//#include "stm32l4xx_hal_rcc.h"

extern u8 MQ2_Air_Quality,MQ135_Air_Quality;

void MY_ADC_Init(void); 				//ADC通道初始化
u16  Get_Adc(u32 ch); 		        //获得某个通道值 
u16 Get_Adc_Average(u32 ch,u8 times);//得到某个通道给定次数采样的平均值
u8 MQ2_Collect(u8 times);
u8 MQ135_Collect(u8 times);

#endif 
