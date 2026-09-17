#ifndef __TIMER_H
#define __TIMER_H
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
void Timer_Init(uint16_t prescaler, uint16_t period);

#endif
