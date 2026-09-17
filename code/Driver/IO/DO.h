#ifndef __DO_SENSOR_H__
#define __DO_SENSOR_H__

#include <stdbool.h>
#include <stdint.h>
#include "delay.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "sys.h"

#include "utility.h"

#define IO_LOW 0
#define IO_HIGH 1
#define DO_OP(dev, op, ...) DO_GetOperations()->op(dev, ##__VA_ARGS__)

// DO 设备结构体
typedef struct
{
	GPIO_TypeDef *gpio_port;
	uint16_t      gpio_pin;
	int           active_level;
	bool          state;
	bool          enabled;
	bool          is_initialized;
} DO_dev_t;
typedef DO_dev_t *DO_handle_t;

// DO 操作接口（ops 风格）
typedef struct
{
	bool (*init)(DO_handle_t hdo);
	bool (*deinit)(DO_handle_t hdo);
	DO_handle_t (*create)(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, int active_level);
	void (*destroy)(DO_handle_t hdo);
	void (*open)(DO_handle_t hdo);
	void (*close)(DO_handle_t hdo);
	void (*toggle)(DO_handle_t hdo);
	void (*set_state)(DO_handle_t hdo, bool state);
	void (*set_enable)(DO_handle_t hdo, bool enable);
	bool (*is_enabled)(DO_handle_t hdo);
	bool (*is_open)(DO_handle_t hdo);
} DO_Operations;

// 基础操作函数
DO_handle_t DO_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, int active_level);

// 销毁函数
void DO_Destroy(DO_handle_t hdo);

DO_Operations *DO_GetOperations(void);

#endif /* __DO_SENSOR_H__ */
