#include "hc_dev.h"
#include <stdbool.h>
#include "delay.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

static bool HC_Trigger(HC_handle_t dev);
static void HC_EnableGPIOClock(GPIO_TypeDef *gpio_port);
static void HC_ConfigTrigGPIO(HC_handle_t dev);
static bool HC_IsValidDistance(float distance_cm);

static HC_dev_t    hc_pool[HC_MAX_DEVICES];
static bool        hc_used[HC_MAX_DEVICES] = {false};
static HC_handle_t hc_instances[HC_MAX_DEVICES] = {NULL};

HC_handle_t HC_Create(GPIO_TypeDef *trig_port, u16 trig_pin, GPIO_TypeDef *echo_port, u16 echo_pin)
{
	HC_handle_t     dev = NULL;
	TIM_IC_handle_t echo_ic = NULL;

	for (int i = 0; i < HC_MAX_DEVICES; i++)
	{
		if (!hc_used[i])
		{
			hc_used[i] = true;
			dev = &hc_pool[i];
			hc_instances[i] = dev;
			break;
		}
	}
	if (dev == NULL)
	{
		return NULL;
	}

	echo_ic = TIM_IC_Create(echo_port, echo_pin);
	if (echo_ic == NULL)
	{
		for (int i = 0; i < HC_MAX_DEVICES; i++)
		{
			if (hc_instances[i] == dev)
			{
				hc_instances[i] = NULL;
				hc_used[i] = false;
				break;
			}
		}
		return NULL;
	}

	dev->trig_port = trig_port;
	dev->trig_pin = trig_pin;
	dev->echo_ic = echo_ic;
	dev->trigger_pulse_us = HC_DEFAULT_TRIGGER_US;
	dev->is_initialized = 1;
	HC_ConfigTrigGPIO(dev);

	return dev;
}

void HC_Destroy(HC_handle_t dev)
{
	if (dev == NULL)
	{
		return;
	}

	if (dev->is_initialized && dev->echo_ic)
	{
		TIM_IC_OP(dev->echo_ic, stop);
		TIM_IC_Destroy(dev->echo_ic);
		dev->echo_ic = NULL;
	}
	dev->is_initialized = 0;

	for (int i = 0; i < HC_MAX_DEVICES; i++)
	{
		if (hc_instances[i] == dev)
		{
			hc_instances[i] = NULL;
			hc_used[i] = false;
			break;
		}
	}
}

static bool HC_Trigger(HC_handle_t dev)
{
	u32 dummy_us = 0;
	u8  drain_guard = 0;

	if (dev == NULL || dev->is_initialized == 0 || dev->echo_ic == NULL)
	{
		return false;
	}

	/* Clear stale capture data to avoid reading previous pulse. */
	while (TIM_IC_OP(dev->echo_ic, read_pulse_us, &dummy_us))
	{
		/* drain one-shot ready flag */
		if (++drain_guard > 8)
		{
			break;
		}
	}

	TIM_IC_OP(dev->echo_ic, start);
	GPIO_ResetBits(dev->trig_port, dev->trig_pin);
	delay_us(2);
	GPIO_SetBits(dev->trig_port, dev->trig_pin);
	delay_us(dev->trigger_pulse_us);
	GPIO_ResetBits(dev->trig_port, dev->trig_pin);
	return true;
}

float HC_ReadDis(HC_handle_t dev, u32 timeout_us)
{
	u32       pulse_us = 0;
	u32       elapsed_us = 0;
	const u32 poll_step_us = 10;

	if (dev == NULL || dev->is_initialized == 0 || dev->echo_ic == NULL)
	{
		return -1.0f;
	}
	if (timeout_us == 0)
	{
		timeout_us = HC_DEFAULT_TIMEOUT_US;
	}
	if (!HC_Trigger(dev))

	{
		return -1.0f;
	}

	while (elapsed_us < timeout_us)
	{
		if (TIM_IC_OP(dev->echo_ic, read_pulse_us, &pulse_us))
		{
			float dis_cm = HC_DISTANCE_CM_FROM_US(pulse_us);
			TIM_IC_OP(dev->echo_ic, stop);
			if (!HC_IsValidDistance(dis_cm))
			{
				return -1.0f;
			}
			return dis_cm;
		}
		delay_us(poll_step_us);
		elapsed_us += poll_step_us;
	}

	TIM_IC_OP(dev->echo_ic, stop);
	return -1.0f;
}

static void HC_EnableGPIOClock(GPIO_TypeDef *gpio_port)
{
	if (gpio_port == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (gpio_port == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
	else if (gpio_port == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if (gpio_port == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}
#endif
}

static void HC_ConfigTrigGPIO(HC_handle_t dev)
{
	GPIO_InitTypeDef gpio;

	HC_EnableGPIOClock(dev->trig_port);
	gpio.GPIO_Pin = dev->trig_pin;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(dev->trig_port, &gpio);
	GPIO_ResetBits(dev->trig_port, dev->trig_pin);
}

static bool HC_IsValidDistance(float distance_cm)
{
	if (distance_cm < 2.0f)
	{
		return false;
	}
	if (distance_cm > 450.0f)
	{
		return false;
	}
	return true;
}
