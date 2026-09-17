#ifndef _JW01_H_
#define _JW01_H_
#include "stm32f10x.h"

#define JW01_USART1 1
#define JW01_USART2 0
#define JW01_USART3 0

#define JW01_USART USART1

#define JW01_3IN1 1
#define JW01_TTYPE_CO2 0

#define JW01_TYPE JW01_3IN1

void JW01_Init(unsigned int bound);
unsigned short JW01_GetCO2(void);
float JW01_GetTVOC(void);
float JW01_GetCH2O(void);

#if JW01_USART1
void USART1_IRQHandler(void);
#elif JW01_USART2
void USART2_IRQHandler(void);
#elif JW01_USART3
void USART3_IRQHandler(void);
#endif

#endif
