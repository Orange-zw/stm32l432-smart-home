#ifndef _USART2_H
#define _USART2_H
#include "sys.h"
#include "stdio.h"	
#include "usart.h"
#include "timer.h"

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART2_RX 			1		//使能（1）/禁止（0）串口1接收
#define GP2Y1050_COLLECT_TIMES  30  //PM2.5模块采集平均次数，每采集该次数求一次平均值，该值最大为64，超过则会造成数据不准

extern UART_HandleTypeDef UART2_Handler; //UART句柄

#define RXBUFFERSIZE   1 //缓存大小
extern u8 aRxBuffer[RXBUFFERSIZE];//HAL库USART接收Buffer

extern u8 GP2Y1050_Times;
extern float GP2Y1050_Quality;

//如果想串口中断接收，请不要注释以下宏定义
void uart2_init(u32 bound);
void uart1_init(u32 pclk2,u32 bound);

#endif
