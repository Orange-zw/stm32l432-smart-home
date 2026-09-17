// sg_90.c
#include "sg_90.h"
#include <stdlib.h>
#include "delay.h"
#include "utility.h"

// 初始化SG90舵机
bool SG90_Init(sg90_handle_t servo)
{
	if (servo == NULL || servo->pwm_driver == NULL)
	{
		return false;
	}

	PWM_Operations *pwm_ops = PWM_GetOperations();

	// 配置为50Hz舵机频率
	// 预分频器=71，周期=19999，时钟72MHz时得到50Hz
	// 定时器时钟 = 72MHz / 72 = 1MHz，每个计数单位 = 1us
	// 周期 = 20000 * 1us = 20000us = 20ms，频率 = 50Hz
	PWM_OP(servo->pwm_driver, setConfig, 720 - 1, 2000 - 1);

	// 启动PWM
	PWM_OP(servo->pwm_driver, start);

	return true;
}

// 创建SG90舵机实例
sg90_handle_t SG90_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, uint8_t default_angle)
{
	sg90_handle_t servo = (sg90_handle_t)malloc(sizeof(sg90_handle_t));
	if (!servo)
	{
		return NULL;
	}

	servo->pwm_driver = PWM_Create(gpio_port, gpio_pin);
	if (!servo->pwm_driver)
	{
		free(servo);
		return NULL;
	}

	servo->current_angle = default_angle;

	SG90_Init(servo);
	SG90_SetAngle(servo, default_angle);
	return servo;
}

// 销毁SG90舵机实例
void SG90_Destroy(sg90_handle_t servo)
{
	if (servo)
	{
		if (servo->pwm_driver)
		{
			PWM_Destroy(servo->pwm_driver);
		}
		free(servo);
	}
}

// 设置舵机角度
void SG90_SetAngle(sg90_handle_t servo, uint8_t angle)
{
	if (servo == NULL || servo->pwm_driver == NULL)
	{
		return;
	}

	angle = _constrain(angle, 0, 180);
	float duty_cycle = 2.5f + (angle * 10.0f) / 180.0f;
	PWM_OP(servo->pwm_driver, setDutyCycle, duty_cycle);
	servo->current_angle = angle;
}

// 获取当前角度
uint8_t SG90_GetAngle(sg90_handle_t servo)
{
	if (servo)
	{
		return servo->current_angle;
	}
	return 0;
}
