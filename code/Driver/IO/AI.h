#ifndef __AI_SENSOR_H__
#define __AI_SENSOR_H__

#include <stdbool.h>
#include <stdint.h>
#include "delay.h"
#include "stm32f10x.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_gpio.h"
#include "sys.h"

#include "utility.h"

#define AI_OP(dev, op, ...) AI_GetOperations()->op(dev, ##__VA_ARGS__)

typedef struct
{
	GPIO_TypeDef *gpio_port;
	uint16_t      gpio_pin;
	ADC_TypeDef  *adc_instance;
	uint8_t       adc_channel;
	bool          is_initialized;
} AI_dev_t;
typedef AI_dev_t *AI_handle_t;

typedef struct
{
	bool (*init)(AI_handle_t hai);
	bool (*deinit)(AI_handle_t hai);
	AI_handle_t (*create)(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, ADC_TypeDef *adc_instance);
	void (*destroy)(AI_handle_t hai);
	uint16_t (*get_raw_value)(AI_handle_t hai);
	float (*get_percent_value)(AI_handle_t hai);
	float (*get_voltage)(AI_handle_t hai, float base_voltage);
} AI_Operations;

AI_handle_t    AI_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, ADC_TypeDef *adc_instance);
void           AI_Destroy(AI_handle_t hai);
AI_Operations *AI_GetOperations(void);

#endif /* __AI_SENSOR_H__ */
