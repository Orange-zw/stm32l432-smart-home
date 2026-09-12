#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"
#include "usart.h"
#include "esp8266.h"

void TIM7_Int_Init(u16 arr,u16 psc);
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim);

#endif
