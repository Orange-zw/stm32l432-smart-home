#ifndef __USART_H
#define __USART_H

#include "stdio.h"	
#include "sys.h" 
#include <stdio.h>

#define Packet_Len  8

#define USART1_ENABLE		    1
#define USART2_ENABLE		    0
#define USART3_ENABLE		    0

#define SEND_USART		        USART1


extern unsigned short data_cnt;
extern uint8_t Seria1_TxPacket[];
extern char Serial_RxPacket[];
extern uint8_t Serial_RxFlag;

void Serial_Iint(u32 bound);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);
void Serial_SendPacket(void);
uint8_t Serial_GetRxFlag(void); 	

#endif


