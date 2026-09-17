#ifndef __AD_H
#define __AD_H
#include "sys.h"

#define PM25_ADC_GPIO_RCC RCC_APB2Periph_GPIOA
#define PM25_ADC_GPIO_PORT GPIOA
#define PM25_ADC_PIN GPIO_Pin_1
#define PM25_ADC_CHANNEL ADC_Channel_1

#define PM25_LED_GPIO_RCC RCC_APB2Periph_GPIOB
#define PM25_LED_GPIO_PORT GPIOB
#define PM25_LED_PIN GPIO_Pin_14

#define PM25_LED PBout(14)

void PM25_Init(void);
uint16_t PM25_GetValue(void);
unsigned int PM2_5_GetData(void); // 定义读取PM2.5的函数

#endif
