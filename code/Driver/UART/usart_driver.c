/* ============================================================
 * 模块     : USART 驱动
 * 功能     : 串口收发、接收回调与缓冲区管理
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "usart_driver.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define USART_MAX_COUNT (5)  // USART实例数组长度

// 私有函数声明
static void USART_GPIO_Init(USART_handle_t huart);
static void USART_NVIC_Init(USART_handle_t huart);
static void USART_Config_Init(USART_handle_t huart);

static bool     USART_driver_Init(USART_handle_t huart);
static bool     USART_driver_Deinit(USART_handle_t huart);
static uint16_t USART_Send(USART_handle_t huart, uint8_t *data, uint16_t length);
static uint16_t USART_Send_fmt(USART_handle_t huart, const char *fmt, ...);
static uint16_t USART_Receive(USART_handle_t huart, uint8_t *buffer, uint16_t length);
static bool     USART_SetRxByteCallback(USART_handle_t huart, USART_RxByteCallback callback);
static bool     USART_SetRxCallback(USART_handle_t huart, USART_RxCompleteCallback callback);
static void     USART_RxBuffer_Write(USART_handle_t huart, uint8_t data);
static uint8_t  USART_RxBuffer_Read(USART_handle_t huart);
static void     USART_TxBuffer_Write(USART_handle_t huart, uint8_t data);
static uint8_t  USART_TxBuffer_Read(USART_handle_t huart);
static uint16_t USART_GetRxData(USART_handle_t huart, uint8_t *buffer, uint16_t length);
static uint16_t USART_GetTxData(USART_handle_t huart, uint8_t *buffer, uint16_t length);
static bool     USART_SetTxCallback(USART_handle_t huart, USART_TxCompleteCallback callback);
static uint16_t USART_GetRxDataLength(USART_handle_t huart);
static void     USART_FlushRx(USART_handle_t huart);
static void     USART_FlushTx(USART_handle_t huart);
static bool     USART_IsRxReady(USART_handle_t huart);
static bool     USART_IsTxReady(USART_handle_t huart);

// 全局操作接口
static USART_Operations usart_ops = {
    .init = USART_driver_Init,
    .deinit = USART_driver_Deinit,
    .create = USART_Create,
    .destroy = USART_Destroy,
    .send = USART_Send,
    .send_fmt = USART_Send_fmt,
    .receive = USART_Receive,
    .set_rx_byte_callback = USART_SetRxByteCallback,
    .set_rx_idle_callback = USART_SetRxCallback,
    .set_tx_callback = USART_SetTxCallback,
    .get_rx_data_length = USART_GetRxDataLength,
    .get_rx_data = USART_GetRxData,
    .flush_rx = USART_FlushRx,
    .flush_tx = USART_FlushTx,
    .is_rx_ready = USART_IsRxReady,
    .is_tx_ready = USART_IsTxReady};

// USART实例数组（用于中断处理）
static USART_handle_t instances[USART_MAX_COUNT] = {NULL};

static void USART_GPIO_Init(USART_handle_t huart)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	if (huart->instance == USART1)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

		// USART1 TX: PA9, RX: PA10
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
	else if (huart->instance == USART2)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

		// USART2 TX: PA2, RX: PA3
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
	else if (huart->instance == USART3)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

		// USART3 TX: PB10, RX: PB11
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOB, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}
}

static void USART_NVIC_Init(USART_handle_t huart)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	if (huart->instance == USART1)
	{
		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	}
	else if (huart->instance == USART2)
	{
		NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	}
	else if (huart->instance == USART3)
	{
		NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	}

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void USART_Config_Init(USART_handle_t huart)
{
	USART_InitTypeDef USART_InitStructure;

	USART_InitStructure.USART_BaudRate = huart->config.baud_rate;
	USART_InitStructure.USART_WordLength = huart->config.data_bits;
	USART_InitStructure.USART_StopBits = huart->config.stop_bits;
	USART_InitStructure.USART_Parity = huart->config.parity;
	USART_InitStructure.USART_Mode = huart->config.mode;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

	USART_Init(huart->instance, &USART_InitStructure);

	// 使能接收中断和空闲中断
	USART_ITConfig(huart->instance, USART_IT_RXNE, ENABLE);
	USART_ITConfig(huart->instance, USART_IT_IDLE, ENABLE);

	USART_Cmd(huart->instance, ENABLE);
}

static void USART_RxBuffer_Write(USART_handle_t huart, uint8_t data)
{
	huart->rx_buffer[huart->rx_write_index] = data;
	huart->rx_write_index = (huart->rx_write_index + 1) % huart->rx_buffer_size;
	huart->rx_count++;
}

static uint8_t USART_RxBuffer_Read(USART_handle_t huart)
{
	uint8_t data = huart->rx_buffer[huart->rx_read_index];
	huart->rx_read_index = (huart->rx_read_index + 1) % huart->rx_buffer_size;
	return data;
}

static void USART_TxBuffer_Write(USART_handle_t huart, uint8_t data)
{
	huart->tx_buffer[huart->tx_write_index] = data;
	huart->tx_write_index = (huart->tx_write_index + 1) % huart->tx_buffer_size;
}

static uint8_t USART_TxBuffer_Read(USART_handle_t huart)
{
	uint8_t data = huart->tx_buffer[huart->tx_read_index];
	huart->tx_read_index = (huart->tx_read_index + 1) % huart->tx_buffer_size;
	return data;
}

bool USART_driver_Init(USART_handle_t huart)
{
	if (huart->is_initialized)
		return true;

	// 初始化缓冲区
	huart->rx_buffer_size = sizeof(huart->rx_buffer);
	huart->tx_buffer_size = sizeof(huart->tx_buffer);
	huart->rx_write_index = 0;
	huart->rx_read_index = 0;
	huart->tx_write_index = 0;
	huart->tx_read_index = 0;

	// 初始化硬件
	USART_GPIO_Init(huart);
	USART_Config_Init(huart);
	USART_NVIC_Init(huart);

	huart->state = USART_STATE_READY;
	huart->is_initialized = true;
	huart->rx_busy = false;
	huart->tx_busy = false;

	// 注册实例（找到第一个空位置存储）
	for (int i = 0; i < USART_MAX_COUNT; i++)
	{
		if (instances[i] == NULL)
		{
			instances[i] = huart;
			break;
		}
	}

	return true;
}

bool USART_driver_Deinit(USART_handle_t huart)
{
	if (!huart->is_initialized)
		return true;

	USART_Cmd(huart->instance, DISABLE);
	USART_ITConfig(huart->instance, USART_IT_RXNE, DISABLE);
	USART_ITConfig(huart->instance, USART_IT_IDLE, DISABLE);

	huart->is_initialized = false;
	huart->state = USART_STATE_READY;

	return true;
}

USART_handle_t USART_Create(USART_TypeDef *instance, uint32_t baud_rate)
{
	USART_handle_t huart = (USART_handle_t)malloc(sizeof(USART_dev_t));
	if (huart == NULL)
		return NULL;

	huart->instance = instance;
	huart->config.baud_rate = baud_rate;
	huart->config.data_bits = USART_WordLength_8b;
	huart->config.stop_bits = USART_StopBits_1;
	huart->config.parity = USART_Parity_No;
	huart->config.mode = USART_Mode_Rx | USART_Mode_Tx;

	// 初始化状态
	huart->state = USART_STATE_READY;
	huart->is_initialized = false;
	huart->rx_busy = false;
	huart->tx_busy = false;
	huart->rx_byte_callback = NULL;
	huart->rx_complete_callback = NULL;
	huart->tx_complete_callback = NULL;
	if (!USART_driver_Init(huart))
	{
		free(huart);
		return NULL;
	}

	return huart;
}

void USART_Destroy(USART_handle_t huart)
{
	if (huart != NULL)
	{
		USART_driver_Deinit(huart);

		// 从实例数组中移除
		for (int i = 0; i < USART_MAX_COUNT; i++)
		{
			if (instances[i] == huart)
			{
				instances[i] = NULL;
				break;
			}
		}

		free(huart);
	}
}

uint16_t USART_Send(USART_handle_t huart, uint8_t *data, uint16_t length)
{
	if (!huart->is_initialized || huart->tx_busy)
		return 0;

	huart->tx_busy = true;
	uint16_t sent = 0;

	for (uint16_t i = 0; i < length; i++)
	{
		// 等待发送缓冲区空
		while (USART_GetFlagStatus(huart->instance, USART_FLAG_TXE) == RESET);

		USART_SendData(huart->instance, data[i]);
		sent++;
		huart->tx_count++;
	}

	// 等待发送完成
	while (USART_GetFlagStatus(huart->instance, USART_FLAG_TC) == RESET);

	huart->tx_busy = false;

	// 调用发送完成回调
	if (huart->tx_complete_callback)
	{
		huart->tx_complete_callback();
	}

	return sent;
}

uint16_t USART_Send_fmt(USART_handle_t huart, const char *fmt, ...)
{
	if (!huart->is_initialized || huart->tx_busy)
		return 0;

	static char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	return USART_Send(huart, (uint8_t *)buffer, strlen(buffer));
}

uint16_t USART_Receive(USART_handle_t huart, uint8_t *buffer, uint16_t length)
{
	if (!huart->is_initialized)
		return 0;

	uint16_t received = 0;
	uint16_t available = USART_GetRxDataLength(huart);

	if (available == 0)
		return 0;

	if (length > available)
	{
		length = available;
	}

	for (uint16_t i = 0; i < length; i++)
	{
		buffer[i] = USART_RxBuffer_Read(huart);
		received++;
	}

	return received;
}

bool USART_SetRxByteCallback(USART_handle_t huart, USART_RxByteCallback callback)
{
	if (!huart->is_initialized)
		return false;

	huart->rx_byte_callback = callback;
	return true;
}

bool USART_SetRxCallback(USART_handle_t huart, USART_RxCompleteCallback callback)
{
	if (!huart->is_initialized)
		return false;

	huart->rx_complete_callback = callback;
	return true;
}

bool USART_SetTxCallback(USART_handle_t huart, USART_TxCompleteCallback callback)
{
	if (!huart->is_initialized)
		return false;

	huart->tx_complete_callback = callback;
	return true;
}

uint16_t USART_GetRxDataLength(USART_handle_t huart)
{
	if (huart->rx_write_index >= huart->rx_read_index)
	{
		return huart->rx_write_index - huart->rx_read_index;
	}
	else
	{
		return huart->rx_buffer_size - huart->rx_read_index + huart->rx_write_index;
	}
}

uint16_t USART_GetRxData(USART_handle_t huart, uint8_t *buffer, uint16_t length)
{
	length = length == 0 ? USART_GetRxDataLength(huart) : length;

	return USART_Receive(huart, buffer, length);
}

void USART_FlushRx(USART_handle_t huart)
{
	memset(huart->rx_buffer, 0, huart->rx_buffer_size);
	huart->rx_write_index = 0;
	huart->rx_read_index = 0;
}

void USART_FlushTx(USART_handle_t huart)
{
	huart->tx_write_index = 0;
	huart->tx_read_index = 0;
}

bool USART_IsRxReady(USART_handle_t huart)
{
	return USART_GetRxDataLength(huart) > 0;
}

bool USART_IsTxReady(USART_handle_t huart)
{
	return !huart->tx_busy;
}

void USART_IRQHandler(USART_handle_t huart)
{
	if (USART_GetITStatus(huart->instance, USART_IT_RXNE) != RESET)
	{
		// 接收数据中断
		uint8_t data = USART_ReceiveData(huart->instance);
		USART_RxBuffer_Write(huart, data);
		USART_ClearITPendingBit(huart->instance, USART_IT_RXNE);

		// 调用接收字节回调
		if (huart->rx_byte_callback)
		{
			huart->rx_byte_callback(&data);
		}
	}
	else if (USART_GetITStatus(huart->instance, USART_IT_IDLE) != RESET)
	{
		// 空闲中断 - 表示一帧数据接收完成
		USART_ReceiveData(huart->instance);  // 读DR寄存器清除空闲中断标志
		USART_ClearITPendingBit(huart->instance, USART_IT_IDLE);

		// 调用接收完成回调
		if (huart->rx_complete_callback)
		{
			uint16_t length = USART_GetRxDataLength(huart);
			if (length > 0)
			{
				huart->rx_complete_callback(huart->rx_buffer, length);
			}
			USART_FlushRx(huart);  // 清空接收缓冲区
		}
	}
	else if (USART_GetITStatus(huart->instance, USART_IT_TXE) != RESET)
	{
		// 发送缓冲区空中断（如果使用中断发送）
		// 这里可以实现中断发送功能
		USART_ClearITPendingBit(huart->instance, USART_IT_TXE);
	}
}

inline USART_Operations *USART_GetOperations(void)
{
	return &usart_ops;
}

USART_handle_t USART_GetInstance(USART_TypeDef *instance)
{
	// 遍历数组查找匹配的实例
	for (int i = 0; i < USART_MAX_COUNT; i++)
	{
		if (instances[i] != NULL && instances[i]->instance == instance)
		{
			return instances[i];
		}
	}
	return NULL;
}

/* 中断接收函数 */
// void USART1_IRQHandler(void)
// {
// 	USART_handle_t huart = USART_GetInstance(USART1);
// 	if (huart != NULL)
// 	{
// 		USART_IRQHandler(huart);
// 	}
// }

// void USART2_IRQHandler(void)
// {
// 	USART_handle_t huart = USART_GetInstance(USART2);
// 	if (huart != NULL)
// 	{
// 		USART_IRQHandler(huart);
// 	}
// }

// void USART3_IRQHandler(void)
// {
// 	USART_handle_t huart = USART_GetInstance(USART3);
// 	if (huart != NULL)
// 	{
// 		USART_IRQHandler(huart);
// 	}
// }
