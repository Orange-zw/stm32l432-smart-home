#ifndef __SU_03T_H
#define __SU_03T_H

#include "stdio.h"
#include "sys.h"
#include <stdio.h>

#define Packet_Len 8

#define SU_03T_USART1 1
#define SU_03T_USART2 0
#define SU_03T_USART3 0

#define SU_03T_USART USART1

extern unsigned char data_buf[];
extern unsigned short data_cnt;

extern uint8_t SU_03T_RxPacket[];

extern uint8_t library_TxPacket[Packet_Len];
extern uint8_t bathhouse_TxPacket[Packet_Len];
extern uint8_t canteen_TxPacket[Packet_Len];
extern uint8_t store_TxPacket[Packet_Len];
extern uint8_t fandian_TxPacket[Packet_Len];

void SU_03T_Init(u32 bound);
void SU_03T_SendByte(uint8_t Byte);
void SU_03T_SendArray(uint8_t *Array, uint16_t Length);
void SU_03T_SendString(char *String);
void SU_03T_SendNumber(uint32_t Number, uint8_t Length);
void SU_03T_Printf(char *format, ...);

extern volatile uint8_t receive_data_buf[];
extern volatile uint8_t receive_data_cnt;
extern volatile uint8_t receive_data_flag;
extern volatile unsigned long su03t_rx_total;
extern volatile uint8_t su03t_rx_last;
uint8_t SU_03T_GetRxFlag(void);
uint8_t SU_03T_ClearRxFlag(void);

#endif
