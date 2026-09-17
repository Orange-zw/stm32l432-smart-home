/* ============================================================
 * 模块     : SU-03T 离线语音模块驱动
 * 功能     : 串口接收语音命令帧、解析命令号并反向发送控制指令
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "sys.h"
#include <stdio.h>
#include <stdarg.h>
#include "su_03t.h"
#include "OLED.h"
#include <string.h>
#include "stdio.h"

//////////////////////////////////////////////////////////////////////////////////
// 如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h" //ucos 使用
#endif

volatile uint8_t receive_data_buf[50];
volatile uint8_t receive_data_cnt = 0;
volatile uint8_t receive_data_flag = 0;
volatile unsigned long su03t_rx_total = 0;
volatile uint8_t su03t_rx_last = 0;
uint8_t receive_state = 0;

uint8_t library_TxPacket[Packet_Len] = {0xAA, 0x55, 0x01, 0X55, 0xAA};
uint8_t bathhouse_TxPacket[Packet_Len] = {0xAA, 0x55, 0x02, 0X55, 0xAA};
uint8_t canteen_TxPacket[Packet_Len] = {0xAA, 0x55, 0x03, 0X55, 0xAA};
uint8_t store_TxPacket[Packet_Len] = {0xAA, 0x55, 0x04, 0X55, 0xAA};
uint8_t fandian_TxPacket[Packet_Len] = {0xAA, 0x55, 0x06, 0X55, 0xAA};

uint8_t SU_03T_RxPacket[Packet_Len];

uint8_t SU_03T_RxFlag;

extern int mode; // 1为自动 2为手动

void SU_03T_Init(u32 bound)
{
#if SU_03T_USART1
	// GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE); // 使能USART1，GPIOA时钟

	// USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);			// 初始化GPIOA.9

	// USART1_RX	  GPIOA.10初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;			  // PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);				  // 初始化GPIOA.10

	// Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  // 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							  // 根据指定的参数初始化VIC寄存器

	// USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;										// 串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 收发模式

	USART_Init(USART1, &USART_InitStructure);	   // 初始化串口1
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 开启串口接受中断
	USART_Cmd(USART1, ENABLE);					   // 使能串口1

#elif SU_03T_USART2
	GPIO_InitTypeDef gpio_initstruct;
	USART_InitTypeDef usart_initstruct;
	NVIC_InitTypeDef nvic_initstruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	// PA2	TXD
	gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_2;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_initstruct);

	// PA3	RXD
	gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_3;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_initstruct);

	usart_initstruct.USART_BaudRate = bound;
	usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
	usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;				 // 接收和发送
	usart_initstruct.USART_Parity = USART_Parity_No;							 // 无校验
	usart_initstruct.USART_StopBits = USART_StopBits_1;							 // 1位停止位
	usart_initstruct.USART_WordLength = USART_WordLength_8b;					 // 8位数据位
	USART_Init(USART2, &usart_initstruct);

	USART_Cmd(USART2, ENABLE); // 使能串口

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // 使能接收中断

	nvic_initstruct.NVIC_IRQChannel = USART2_IRQn;
	nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
	nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 1;
	nvic_initstruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic_initstruct);

#elif SU_03T_USART3
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	USART_DeInit(USART3); // 复位串?
	// USART2_TX   PB10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // PB10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);			// 初始化PB10

	// USART3_RX	  PB11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);				  // 初始化PB11

	USART_InitStructure.USART_BaudRate = bound;										// 一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 收发模式

	USART_Init(USART3, &USART_InitStructure); // 初始化串口	3

	USART_Cmd(USART3, ENABLE); // 使能串口
	// 使能接收中断
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); // 开启中断

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		  // 子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							  // 根据指定的参数初始化VIC寄存器
#endif
}

extern u16 online_count;

#if SU_03T_USART1
void USART1_IRQHandler(void)
{
	uint8_t RxData;

	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		RxData = USART_ReceiveData(USART1);
		su03t_rx_total++;
		su03t_rx_last = RxData;

		// If previous command is not consumed yet, drop incoming bytes.
		if (receive_data_flag)
		{
			USART_ClearITPendingBit(USART1, USART_IT_RXNE);
			return;
		}

		if (receive_data_cnt == 0)
		{
			if (RxData == 0xAA)
			{
				receive_data_buf[receive_data_cnt++] = RxData;
			}
			else if (RxData >= 0x01 && RxData <= 0x0E)
			{
				receive_data_buf[receive_data_cnt++] = RxData;
				receive_data_flag = 1;
			}
		}
		else
		{
			if (receive_data_cnt < sizeof(receive_data_buf))
			{
				receive_data_buf[receive_data_cnt++] = RxData;
			}
			else
			{
				receive_data_cnt = 0;
			}

			// Frame path: AA 55 CMD 55 AA
			if (receive_data_cnt == 2 && receive_data_buf[0] == 0xAA && receive_data_buf[1] != 0x55)
			{
				uint8_t retry_byte = receive_data_buf[1];
				receive_data_cnt = 0;
				if (retry_byte == 0xAA)
				{
					receive_data_buf[receive_data_cnt++] = retry_byte;
				}
				else if (retry_byte >= 0x01 && retry_byte <= 0x0E)
				{
					receive_data_buf[receive_data_cnt++] = retry_byte;
					receive_data_flag = 1;
				}
			}
			else if (receive_data_cnt >= 5 &&
			         receive_data_buf[0] == 0xAA &&
			         receive_data_buf[1] == 0x55 &&
			         receive_data_buf[receive_data_cnt - 2] == 0x55 &&
			         receive_data_buf[receive_data_cnt - 1] == 0xAA)
			{
				receive_data_flag = 1;
			}
		}

		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}
#elif SU_03T_USART2
void USART2_IRQHandler(void)
{
	static uint8_t RxState = 0;
	static uint8_t pRxPacket = 0;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		uint8_t RxData = USART_ReceiveData(USART2);
		// 状态机处理接收到的数据
		switch (RxState)
		{
		case 0:								  // 等待起始字节
			if (RxData == Seria1_TxPacket[0]) // 检查起始字节
			{
				SU_03T_RxPacket[pRxPacket++] = RxData;
				RxState = 1; // 进入下一个状态
			}
			break;

		case 1: // 接收数据
			SU_03T_RxPacket[pRxPacket++] = RxData;
			if (pRxPacket == sizeof(SU_03T_RxPacket)) // 检查是否接收完一帧数据
			{
				SU_03T_RxFlag = 1;
				// 数据接收完成，可以在这里进行处理
				pRxPacket = 0; // 重置索引以准备接收下一帧
				RxState = 0;   // 回到初始状态
			}
			break;
		}

		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}
}

#elif SU_03T_USART3
void USART3_IRQHandler(void)
{
	//    static uint8_t RxState = 0;
	//    static uint8_t pRxPacket = 0;

	if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	{
		uint8_t RxData = USART_ReceiveData(USART3);
		switch (1)
		{
		}
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}
#endif

void SU_03T_SendByte(uint8_t Byte)
{
	USART_SendData(SU_03T_USART, Byte);
	while (USART_GetFlagStatus(SU_03T_USART, USART_FLAG_TXE) == RESET)
		;
}

void SU_03T_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	uint8_t frame_head[2] = {0xAA, 0x55};
	uint8_t frame_tail[2] = {0x55, 0xAA};
	SU_03T_SendByte(frame_head[0]);
	SU_03T_SendByte(frame_head[1]);
	for (i = 0; i < Length; i++)
	{
		SU_03T_SendByte(Array[i]);
	}
	SU_03T_SendByte(frame_tail[0]);
	SU_03T_SendByte(frame_tail[1]);
}

void SU_03T_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		SU_03T_SendByte(String[i]);
	}
}

uint32_t SU_03T_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

void SU_03T_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		SU_03T_SendByte(Number / SU_03T_Pow(10, Length - i - 1) % 10 + '0');
	}
}

void SU_03T_Printf(char *format, ...)
{
	char String[100];
	va_list arg;
	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);
	SU_03T_SendString(String);
}

uint8_t SU_03T_GetRxFlag(void)
{
	// if (SU_03T_RxFlag == 1)
	// {
	// 	SU_03T_RxFlag = 0;
	// 	return 1;
	// }
	// return 0;
	return receive_data_flag;
}

uint8_t SU_03T_ClearRxFlag(void)
{
	receive_data_flag = 0;
	receive_data_cnt = 0;
	return 0;
}