// sg_90.h
#ifndef __SG_90_H__
#define __SG_90_H__

#include <stdbool.h>
#include <stdint.h>
#include "pwm_driver.h"
#include "stm32f10x.h"

// SG90舵机结构体
typedef struct
{
	PWM_Handle_t *pwm_driver;
	uint8_t       current_angle;
} sg90_dev_t;
typedef sg90_dev_t *sg90_handle_t;

// 函数声明
sg90_handle_t SG90_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, uint8_t default_angle);
void          SG90_Destroy(sg90_handle_t servo);
void          SG90_SetAngle(sg90_handle_t servo, uint8_t angle);
uint8_t       SG90_GetAngle(sg90_handle_t servo);

#endif /* __SG_90_H__ */