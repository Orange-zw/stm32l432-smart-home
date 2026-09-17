// PWM_dev.h
#ifndef __PWM_DEV_H__
#define __PWM_DEV_H__

#include <stdbool.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"


#define PWM_OP(dev, op, ...) PWM_GetOperations()->op(dev, ##__VA_ARGS__)

// PWM映射配置结构体
typedef struct
{
	GPIO_TypeDef *gpio_port;
	uint16_t      gpio_pin;
	TIM_TypeDef  *timer;
	uint16_t      channel;
	bool          is_complementary;  // 是否为互补通道（CHxN）
	bool          is_remapped;       // 是否为重映射通道
	uint32_t      remap_value;       // 重映射值
} PWM_PinMap;

// PWM驱动结构体
typedef struct
{
	TIM_TypeDef  *timer;
	uint16_t      channel;
	uint32_t      frequency;
	uint16_t      duty_cycle;
	bool          enabled;
	GPIO_TypeDef *gpio_port;
	uint16_t      gpio_pin;
	bool          is_complementary;  // 是否为互补通道（CHxN）
} PWM_Handle_t;

// 操作函数指针类型定义
typedef void (*PWM_StartFunc)(PWM_Handle_t *driver);
typedef void (*PWM_StopFunc)(PWM_Handle_t *driver);
typedef void (*PWM_SetDutyFunc)(PWM_Handle_t *driver, float duty_cycle);
typedef void (*PWM_SetFreqFunc)(PWM_Handle_t *driver, uint32_t frequency);
typedef bool (*PWM_GetStateFunc)(PWM_Handle_t *driver);
typedef void (*PWM_SetConfigFunc)(PWM_Handle_t *driver, uint32_t prescaler, uint32_t period);
typedef uint16_t (*PWM_GetDutyFunc)(PWM_Handle_t *driver);
typedef uint32_t (*PWM_GetFreqFunc)(PWM_Handle_t *driver);
typedef uint32_t (*PWM_GetPrescalerFunc)(PWM_Handle_t *driver);
typedef uint32_t (*PWM_GetPeriodFunc)(PWM_Handle_t *driver);

// PWM操作接口结构体
typedef struct
{
	PWM_StartFunc        start;
	PWM_StopFunc         stop;
	PWM_SetDutyFunc      setDutyCycle;
	PWM_SetFreqFunc      setFrequency;
	PWM_SetConfigFunc    setConfig;
	PWM_GetStateFunc     isEnabled;
	PWM_GetDutyFunc      getDuty;
	PWM_GetFreqFunc      getFrequency;
	PWM_GetPrescalerFunc getPrescaler;
	PWM_GetPeriodFunc    getPeriod;
} PWM_Operations;

// 创建PWM驱动
PWM_Handle_t *PWM_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin);

// 销毁PWM驱动
void PWM_Destroy(PWM_Handle_t *driver);

// 获取PWM操作接口
PWM_Operations *PWM_GetOperations(void);

#endif /* __PWM_DEV_H__ */