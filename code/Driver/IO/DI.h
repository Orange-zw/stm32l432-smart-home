#ifndef __DI_SENSOR_H__
#define __DI_SENSOR_H__

#include <stdbool.h>
#include <stdint.h>
#include "delay.h"
#include "misc.h"
#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "sys.h"

#include "utility.h"

#define IO_LOW 0
#define IO_HIGH 1
#define DI_OP(dev, op, ...) DI_GetOperations()->op(dev, ##__VA_ARGS__)

// 边沿事件类型
typedef enum
{
	DI_EVENT_RISING = 0,  // 上升沿
	DI_EVENT_FALLING,     // 下降沿
	DI_EVENT_BOTH         // 双边沿
} DI_EventType;

typedef struct DI_dev_t DI_dev_t;
typedef DI_dev_t       *DI_handle_t;

// 回调函数类型定义
typedef void (*DI_Callback)(DI_handle_t hdi, DI_EventType event, void *args);

struct DI_dev_t
{
	GPIO_TypeDef *gpio_port;
	uint16_t      gpio_pin;
	int           active_level;  // 有效电平：0-低电平有效，1-高电平有效
	bool          state;         // 当前状态
	bool          last_state;    // 上一个状态

	DI_Callback rising_callback;   // 上升沿回调
	DI_Callback falling_callback;  // 下降沿回调
	DI_Callback both_callback;     // 双边沿回调
	void       *rising_args;       // 上升沿回调参数
	void       *falling_args;      // 下降沿回调参数
	void       *both_args;         // 双边沿回调参数
	uint32_t    exti_line;         // EXTI线
	uint8_t     irq_channel;       // 中断通道
	IRQn_Type   irqn;              // 中断号（用于NVIC配置）
	bool        is_initialized;
};

// DI 操作接口（ops 风格）
typedef struct
{
	bool (*init)(DI_handle_t hdi);
	bool (*deinit)(DI_handle_t hdi);
	DI_handle_t (*create)(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, int active_level);
	void (*destroy)(DI_handle_t hdi);
	bool (*read)(DI_handle_t hdi);                                                                // 读取当前电平状态
	bool (*is_active)(DI_handle_t hdi);                                                           // 判断是否处于有效状态
	int (*get_active_state)(DI_handle_t hdi);                                                     // 获取有效状态
	void (*enable_irq)(DI_handle_t hdi, bool enable);                                             // 设置中断使能
	void (*set_callback)(DI_handle_t hdi, DI_EventType event, DI_Callback callback, void *args);  // 设置回调函数
} DI_Operations;

// 创建 DI 传感器设备
DI_handle_t DI_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, int active_level);

// 销毁 DI 传感器设备
void DI_Destroy(DI_handle_t hdi);

DI_Operations *DI_GetOperations(void);

#endif /* __DI_SENSOR_H__ */