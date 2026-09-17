/* ============================================================
 * 模块     : AI 模拟输入驱动
 * 功能     : 模拟量输入的采集与量程换算
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "AI.h"
#include <stdlib.h>

static uint8_t ADC1_Initialized = 0;
static uint8_t ADC2_Initialized = 0;

typedef struct
{
	GPIO_TypeDef *port;
	uint16_t      pin;
	uint8_t       channel;
} ADC_PinChannelMap;

static const ADC_PinChannelMap adc_pin_channel_map[] = {
    {GPIOA, GPIO_Pin_0, ADC_Channel_0},
    {GPIOA, GPIO_Pin_1, ADC_Channel_1},
    {GPIOA, GPIO_Pin_2, ADC_Channel_2},
    {GPIOA, GPIO_Pin_3, ADC_Channel_3},
    {GPIOA, GPIO_Pin_4, ADC_Channel_4},
    {GPIOA, GPIO_Pin_5, ADC_Channel_5},
    {GPIOA, GPIO_Pin_6, ADC_Channel_6},
    {GPIOA, GPIO_Pin_7, ADC_Channel_7},
    {GPIOB, GPIO_Pin_0, ADC_Channel_8},
    {GPIOB, GPIO_Pin_1, ADC_Channel_9},
};

static bool     AI_Driver_Init(AI_handle_t hai);
static bool     AI_Driver_Deinit(AI_handle_t hai);
static uint16_t AI_GetRawValue(AI_handle_t hai);
static float    AI_GetPercentValue(AI_handle_t hai);
static float    AI_GetVoltage(AI_handle_t hai, float base_voltage);

static AI_Operations ai_ops = {
    .init = AI_Driver_Init,
    .deinit = AI_Driver_Deinit,
    .create = AI_Create,
    .destroy = AI_Destroy,
    .get_raw_value = AI_GetRawValue,
    .get_percent_value = AI_GetPercentValue,
    .get_voltage = AI_GetVoltage};

static void ADC_InitOnce(ADC_TypeDef *ADCx)
{
	ADC_InitTypeDef ADC_InitStructure;

	if (ADCx == ADC1 && ADC1_Initialized)
		return;
	if (ADCx == ADC2 && ADC2_Initialized)
		return;

	if (ADCx == ADC1)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
		ADC1_Initialized = 1;
	}
	else if (ADCx == ADC2)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
		ADC2_Initialized = 1;
	}
	else
	{
		return;
	}

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADCx, &ADC_InitStructure);

	ADC_Cmd(ADCx, ENABLE);

	ADC_ResetCalibration(ADCx);
	while (ADC_GetResetCalibrationStatus(ADCx));
	ADC_StartCalibration(ADCx);
	while (ADC_GetCalibrationStatus(ADCx));
}

static uint8_t AI_FindAdcChannel(GPIO_TypeDef *gpio_port, uint16_t gpio_pin)
{
	for (size_t i = 0; i < sizeof(adc_pin_channel_map) / sizeof(adc_pin_channel_map[0]); i++)
	{
		if (adc_pin_channel_map[i].port == gpio_port && adc_pin_channel_map[i].pin == gpio_pin)
		{
			return adc_pin_channel_map[i].channel;
		}
	}
	return 0xFF;
}

static void AI_GPIO_Init(AI_handle_t hai)
{
	if (hai->gpio_port == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (hai->gpio_port == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if (hai->gpio_port == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if (hai->gpio_port == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = hai->gpio_pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(hai->gpio_port, &GPIO_InitStructure);
}

static bool AI_Driver_Init(AI_handle_t hai)
{
	if (hai == NULL)
		return false;
	if (hai->is_initialized)
		return true;

	AI_GPIO_Init(hai);
	if (hai->adc_instance != NULL)
	{
		ADC_InitOnce(hai->adc_instance);
	}
	hai->is_initialized = true;
	return true;
}

static bool AI_Driver_Deinit(AI_handle_t hai)
{
	if (hai == NULL)
		return false;
	if (!hai->is_initialized)
		return true;

	hai->is_initialized = false;
	return true;
}

static uint16_t AI_GetRawValue(AI_handle_t hai)
{
	if (hai == NULL || hai->adc_instance == NULL || hai->adc_channel == 0xFF)
	{
		return 0;
	}

	ADC_RegularChannelConfig(hai->adc_instance, hai->adc_channel, 1, ADC_SampleTime_55Cycles5);
	ADC_ClearFlag(hai->adc_instance, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(hai->adc_instance, ENABLE);
	while (ADC_GetFlagStatus(hai->adc_instance, ADC_FLAG_EOC) == RESET);
	return ADC_GetConversionValue(hai->adc_instance);
}

static float AI_GetPercentValue(AI_handle_t hai)
{
	uint16_t raw_value = AI_GetRawValue(hai);
	return (float)(raw_value * 100.0f) / 4095.0f;
}

static float AI_GetVoltage(AI_handle_t hai, float base_voltage)
{
	uint16_t raw_value = AI_GetRawValue(hai);
	return (float)(raw_value * base_voltage) / 4095.0f;
}

AI_handle_t AI_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, ADC_TypeDef *adc_instance)
{
	AI_handle_t hai = (AI_handle_t)malloc(sizeof(AI_dev_t));
	if (hai == NULL)
		return NULL;

	hai->gpio_port = gpio_port;
	hai->gpio_pin = gpio_pin;
	hai->adc_instance = adc_instance;
	hai->adc_channel = AI_FindAdcChannel(gpio_port, gpio_pin);
	hai->is_initialized = false;

	if (!AI_Driver_Init(hai))
	{
		free(hai);
		return NULL;
	}
	return hai;
}

void AI_Destroy(AI_handle_t hai)
{
	if (hai == NULL)
		return;
	AI_Driver_Deinit(hai);
	free(hai);
}

AI_Operations *AI_GetOperations(void)
{
	return &ai_ops;
}
