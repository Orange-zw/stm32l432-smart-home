#ifndef	__CONTROL_H
#define	__CONTROL_H

#include "stm32f10x.h"                  // Device header
void beep_init(void);
void human_init(void) ;
#define beep PAout(8)// PA8
#define REN  PBin(13)// PB13
void USART3_Config(void);
void CO2GetData(uint16_t *data);

#endif
