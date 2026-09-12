#ifndef _USART_H
#define _USART_H
#include "sys.h"
#include "stdio.h"	
#include "usart2.h"
#include "delay.h"
#include "timer.h"
#include "esp8266.h"

//串口1相关宏定义
#define ESP8266_MAX_RECV_LEN		800					//最大接收缓存字节数
#define ESP8266_MAX_SEND_LEN		800					//最大发送缓存字节数
#define ESP8266_RX_EN 			1					//0,不接收;1,接收.

//GPIO设置专用宏定义
#define GPIO_MODE_IN    	0		//普通输入模式
#define GPIO_MODE_OUT		1		//普通输出模式
#define GPIO_MODE_AF		2		//AF功能模式
#define GPIO_MODE_AIN		3		//模拟输入模式

#define GPIO_SPEED_2M		0		//GPIO速度2Mhz
#define GPIO_SPEED_25M		1		//GPIO速度25Mhz
#define GPIO_SPEED_50M		2		//GPIO速度50Mhz
#define GPIO_SPEED_100M		3		//GPIO速度100Mhz

#define GPIO_PUPD_NONE		0		//不带上下拉
#define GPIO_PUPD_PU		1		//上拉
#define GPIO_PUPD_PD		2		//下拉
#define GPIO_PUPD_RES		3		//保留 

#define GPIO_OTYPE_PP		0		//推挽输出
#define GPIO_OTYPE_OD		1		//开漏输出 
//GPIO引脚编号定义
#define PIN0				1<<0
#define PIN1				1<<1
#define PIN2				1<<2
#define PIN3				1<<3
#define PIN4				1<<4
#define PIN5				1<<5
#define PIN6				1<<6
#define PIN7				1<<7
#define PIN8				1<<8
#define PIN9				1<<9
#define PIN10				1<<10
#define PIN11				1<<11
#define PIN12				1<<12
#define PIN13				1<<13
#define PIN14				1<<14
#define PIN15				1<<15 

extern u8  ESP8266_RX_BUF[ESP8266_MAX_RECV_LEN]; 		//接收缓冲,最大USART3_MAX_RECV_LEN字节
extern u8  ESP8266_TX_BUF[ESP8266_MAX_SEND_LEN]; 		//发送缓冲,最大USART3_MAX_SEND_LEN字节
extern u16 ESP8266_RX_STA;   						//接收数据状态
extern UART_HandleTypeDef UART1_Handler; //UART句柄

void uart_init(u32 bound);
void esp8266_printf(char* fmt,...);
void WIFI_SendData(u8* data);

#endif
