#ifndef __LAYA_H
#define __LAYA_H

#include "stdio.h"	
#include "sys.h" 
#include <stdio.h>

#define Laya_Packet_Len     8


#define Laya_USART1		    1
#define Laya_USART2		    0
#define Laya_USART3		    0

#define Laya_USART		    USART1

extern  char receive_data_buf[];
extern unsigned short data_cnt;
extern uint8_t Laya_RxPacket[];
extern uint8_t Laya_RxFlag;

extern uint8_t OVER_TIME_TxPacket[Laya_Packet_Len];
extern uint8_t lENGHT_OUT_TxPacket[Laya_Packet_Len];
extern uint8_t JIUJING_OUT_TxPacket[Laya_Packet_Len];

void Laya_Iint(u32 bound);
void Laya_SendByte(uint8_t Byte);
void Laya_SendArray(uint8_t *Array, uint16_t Length);
void Laya_SendString(char *String);
void Laya_SendNumber(uint32_t Number, uint8_t Length);
void Laya_Printf(char *format, ...);

uint8_t Laya_GetRxFlag(void); 	

#endif


