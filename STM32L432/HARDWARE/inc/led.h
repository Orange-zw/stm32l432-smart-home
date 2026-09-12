#ifndef _LED_H
#define _LED_H
#include "sys.h"
#include "adc.h"

#define AIR_CLEAR_ON 	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET)
#define AIR_CLEAR_OFF HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_SET)

#define EXHAUST_FAN_ON 	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_11,GPIO_PIN_SET)
#define EXHAUST_FAN_OFF HAL_GPIO_WritePin(GPIOA,GPIO_PIN_11,GPIO_PIN_RESET)

#define LIGHT_ON HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET)
#define LIGHT_OFF HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET)

extern u8 Manual_STA;
extern u8 AUTO_STA;
extern u8 PM2_5_Quality;

void LED_Init(void);
void Control(void);
#endif
