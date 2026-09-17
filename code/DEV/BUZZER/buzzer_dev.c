#include "buzzer_dev.h"
#include <stdlib.h>
#include "delay.h"

buzzer_handle_t Buzzer_Create(buzzer_driver_type_t type, GPIO_TypeDef *gpio_port, uint16_t gpio_pin)
{
	buzzer_handle_t buzzer = 0;

	if (gpio_port == 0 || gpio_pin == 0U)
	{
		return 0;
	}

	buzzer = (buzzer_handle_t)malloc(sizeof(buzzer_dev_t));
	if (buzzer == 0)
	{
		return 0;
	}

	buzzer->type = type;
	buzzer->gpio_port = gpio_port;
	buzzer->gpio_pin = gpio_pin;
	buzzer->do_dev = 0;
	buzzer->pwm_dev = 0;
	buzzer->pwm_freq_hz = 1000U;

	if (type == BUZZER_DRV_GPIO)
	{
		buzzer->do_dev = DO_Create(gpio_port, gpio_pin, IO_HIGH);
		if (buzzer->do_dev == 0)
		{
			free(buzzer);
			return 0;
		}
	}
	else if (type == BUZZER_DRV_PWM)
	{
		buzzer->pwm_dev = PWM_Create(gpio_port, gpio_pin);
		if (buzzer->pwm_dev == 0)
		{
			free(buzzer);
			return 0;
		}
		PWM_OP(buzzer->pwm_dev, setFrequency, buzzer->pwm_freq_hz);
		PWM_OP(buzzer->pwm_dev, stop);
	}
	else
	{
		free(buzzer);
		return 0;
	}

	return buzzer;
}

void Buzzer_Destroy(buzzer_handle_t buzzer)
{
	if (buzzer == 0)
	{
		return;
	}

	if (buzzer->do_dev != 0)
	{
		DO_Destroy(buzzer->do_dev);
		buzzer->do_dev = 0;
	}
	if (buzzer->pwm_dev != 0)
	{
		PWM_Destroy(buzzer->pwm_dev);
		buzzer->pwm_dev = 0;
	}

	free(buzzer);
}

void Buzzer_Open(buzzer_handle_t buzzer)
{
	if (buzzer == 0)
	{
		return;
	}

	if (buzzer->type == BUZZER_DRV_GPIO)
	{
		if (buzzer->do_dev == 0)
		{
			return;
		}
		DO_OP(buzzer->do_dev, open);
		return;
	}

	if (buzzer->type == BUZZER_DRV_PWM)
	{
		if (buzzer->pwm_dev == 0 || buzzer->pwm_freq_hz == 0U)
		{
			return;
		}
		PWM_OP(buzzer->pwm_dev, setFrequency, buzzer->pwm_freq_hz);
		/* 50%占空比可产生稳定方波，便于听到频率变化 */
		PWM_OP(buzzer->pwm_dev, setDutyCycle, 50.0f);
		PWM_OP(buzzer->pwm_dev, start);
	}
}

void Buzzer_Close(buzzer_handle_t buzzer)
{
	if (buzzer == 0)
	{
		return;
	}

	if (buzzer->type == BUZZER_DRV_GPIO)
	{
		if (buzzer->do_dev == 0)
		{
			return;
		}
		DO_OP(buzzer->do_dev, close);
		return;
	}

	if (buzzer->type == BUZZER_DRV_PWM)
	{
		if (buzzer->pwm_dev == 0)
		{
			return;
		}
		PWM_OP(buzzer->pwm_dev, stop);
	}
}

void Buzzer_SetFreq(buzzer_handle_t buzzer, uint32_t freq_hz)
{
	if (buzzer == 0 || buzzer->type != BUZZER_DRV_PWM || buzzer->pwm_dev == 0 || freq_hz == 0U)
	{
		return;
	}

	buzzer->pwm_freq_hz = freq_hz;
	PWM_OP(buzzer->pwm_dev, setFrequency, freq_hz);
}

void Buzzer_BeepMs(buzzer_handle_t buzzer, uint32_t duration_ms)
{
	if (buzzer == 0 || buzzer->type != BUZZER_DRV_GPIO || buzzer->do_dev == 0)
	{
		return;
	}

	DO_OP(buzzer->do_dev, open);
	delay_ms(duration_ms);
	DO_OP(buzzer->do_dev, close);
}

void Buzzer_BeepPwm(buzzer_handle_t buzzer, uint32_t duration_ms, uint32_t freq_hz)
{
	if (buzzer == 0 || buzzer->type != BUZZER_DRV_PWM || buzzer->pwm_dev == 0 || freq_hz == 0U)
	{
		return;
	}

	buzzer->pwm_freq_hz = freq_hz;
	PWM_OP(buzzer->pwm_dev, setFrequency, buzzer->pwm_freq_hz);
	PWM_OP(buzzer->pwm_dev, setDutyCycle, 50.0f);
	PWM_OP(buzzer->pwm_dev, start);
	delay_ms(duration_ms);
	PWM_OP(buzzer->pwm_dev, stop);
}
