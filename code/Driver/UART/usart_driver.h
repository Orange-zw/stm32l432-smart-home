#ifndef __USART_DRIVER_H__
#define __USART_DRIVER_H__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

#define USART_OP(dev, op, ...) USART_GetOperations()->op(dev, ##__VA_ARGS__)

// 串口状态枚举
typedef enum
{
	USART_STATE_READY = 0,
	USART_STATE_BUSY,
	USART_STATE_ERROR,
	USART_STATE_TIMEOUT
} USART_State;

// 串口配置结构体
typedef struct
{
	uint32_t baud_rate;
	uint16_t data_bits;
	uint16_t stop_bits;
	uint16_t parity;
	uint16_t mode;
} USART_Config;

// 回调函数类型
typedef void (*USART_RxByteCallback)(uint8_t *data);
typedef void (*USART_RxCompleteCallback)(uint8_t *data, uint16_t length);
typedef void (*USART_TxCompleteCallback)(void);

// 串口句柄结构体
typedef struct
{
	// 硬件配置
	USART_TypeDef *instance;
	USART_Config   config;

	// 缓冲区
	uint8_t  rx_buffer[256];
	uint16_t rx_buffer_size;
	uint16_t rx_write_index;
	uint16_t rx_read_index;

	uint8_t  tx_buffer[256];
	uint16_t tx_buffer_size;
	uint16_t tx_write_index;
	uint16_t tx_read_index;

	// 状态标志
	USART_State state;
	bool        is_initialized;
	bool        rx_busy;
	bool        tx_busy;

	// 回调函数
	USART_RxByteCallback     rx_byte_callback;
	USART_RxCompleteCallback rx_complete_callback;
	USART_TxCompleteCallback tx_complete_callback;

	// 统计信息
	uint32_t rx_count;
	uint32_t tx_count;
	uint32_t error_count;

} USART_dev_t;
typedef USART_dev_t *USART_handle_t;

// 串口操作接口
typedef struct
{
	bool (*init)(USART_handle_t huart);
	bool (*deinit)(USART_handle_t huart);
	USART_handle_t (*create)(USART_TypeDef *instance, uint32_t baud_rate);
	void (*destroy)(USART_handle_t huart);
	uint16_t (*send)(USART_handle_t huart, uint8_t *data, uint16_t length);
	uint16_t (*send_fmt)(USART_handle_t huart, const char *fmt, ...);
	uint16_t (*receive)(USART_handle_t huart, uint8_t *buffer, uint16_t length);
	bool (*set_rx_byte_callback)(USART_handle_t huart, USART_RxByteCallback callback);
	bool (*set_rx_idle_callback)(USART_handle_t huart, USART_RxCompleteCallback callback);
	bool (*set_tx_callback)(USART_handle_t huart, USART_TxCompleteCallback callback);
	uint16_t (*get_rx_data_length)(USART_handle_t huart);
	uint16_t (*get_rx_data)(USART_handle_t huart, uint8_t *buffer, uint16_t length);
	void (*flush_rx)(USART_handle_t huart);
	void (*flush_tx)(USART_handle_t huart);
	bool (*is_rx_ready)(USART_handle_t huart);
	bool (*is_tx_ready)(USART_handle_t huart);
} USART_Operations;

// 基础操作函数
USART_handle_t    USART_Create(USART_TypeDef *instance, uint32_t baud_rate);
void              USART_Destroy(USART_handle_t huart);
USART_Operations *USART_GetOperations(void);

#endif /* __USART_DRIVER_H__ */