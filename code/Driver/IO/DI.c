/* ============================================================
 * 模块     : DI 数字输入驱动
 * 功能     : 数字量输入读取与边沿事件回调（EXTI）
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "DI.h"
#include <stdlib.h>

#define EXTI_MAP_COUNT (16)

static bool DI_Read(DI_handle_t hdi);
static bool DI_IsActive(DI_handle_t hdi);
static int  DI_GetActiveState(DI_handle_t hdi);
static void DI_EnableIRQ(DI_handle_t hdi, bool enable);
static void DI_SetCallback(DI_handle_t hdi, DI_EventType event, DI_Callback callback, void *args);
static bool DI_Driver_Init(DI_handle_t hdi);
static bool DI_Driver_Deinit(DI_handle_t hdi);

// 外部中断配置映射表结构
typedef struct
{
	uint16_t  pin;
	uint32_t  exti_line;
	uint8_t   irq_channel;
	IRQn_Type irqn;
} DI_EXTI_Map;

static const DI_EXTI_Map exti_map[] = {
    {GPIO_Pin_0, EXTI_Line0, 0, EXTI0_IRQn},
    {GPIO_Pin_1, EXTI_Line1, 1, EXTI1_IRQn},
    {GPIO_Pin_2, EXTI_Line2, 2, EXTI2_IRQn},
    {GPIO_Pin_3, EXTI_Line3, 3, EXTI3_IRQn},
    {GPIO_Pin_4, EXTI_Line4, 4, EXTI4_IRQn},
    {GPIO_Pin_5, EXTI_Line5, 5, EXTI9_5_IRQn},
    {GPIO_Pin_6, EXTI_Line6, 6, EXTI9_5_IRQn},
    {GPIO_Pin_7, EXTI_Line7, 7, EXTI9_5_IRQn},
    {GPIO_Pin_8, EXTI_Line8, 8, EXTI9_5_IRQn},
    {GPIO_Pin_9, EXTI_Line9, 9, EXTI9_5_IRQn},
    {GPIO_Pin_10, EXTI_Line10, 10, EXTI15_10_IRQn},
    {GPIO_Pin_11, EXTI_Line11, 11, EXTI15_10_IRQn},
    {GPIO_Pin_12, EXTI_Line12, 12, EXTI15_10_IRQn},
    {GPIO_Pin_13, EXTI_Line13, 13, EXTI15_10_IRQn},
    {GPIO_Pin_14, EXTI_Line14, 14, EXTI15_10_IRQn},
    {GPIO_Pin_15, EXTI_Line15, 15, EXTI15_10_IRQn},
    {0, 0, 0, 0}};

static DI_handle_t sensor_instances[EXTI_MAP_COUNT] = {0};

static DI_Operations di_ops = {
    .init = DI_Driver_Init,
    .deinit = DI_Driver_Deinit,
    .create = DI_Create,
    .destroy = DI_Destroy,
    .read = DI_Read,
    .is_active = DI_IsActive,
    .get_active_state = DI_GetActiveState,
    .enable_irq = DI_EnableIRQ,
    .set_callback = DI_SetCallback};

static uint8_t DI_PinToSource(uint16_t pin)
{
	uint8_t index = 0;
	if (pin == 0)
	{
		return 0;
	}
	while ((pin & 0x01u) == 0u)
	{
		pin >>= 1;
		index++;
	}
	return index;
}

static const DI_EXTI_Map *DI_FindEXTIMap(uint16_t pin)
{
	for (int i = 0; exti_map[i].pin != 0; i++)
	{
		if (exti_map[i].pin == pin)
		{
			return &exti_map[i];
		}
	}
	return NULL;
}

static void DI_GPIO_Init(DI_handle_t hdi)
{
	if (hdi->gpio_port == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (hdi->gpio_port == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if (hdi->gpio_port == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if (hdi->gpio_port == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = hdi->gpio_pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(hdi->gpio_port, &GPIO_InitStructure);
}

static void DI_InitEXTI(DI_handle_t hdi)
{
	const DI_EXTI_Map *map = DI_FindEXTIMap(hdi->gpio_pin);
	if (map == NULL)
	{
		return;
	}

	hdi->exti_line = map->exti_line;
	hdi->irq_channel = map->irq_channel;
	hdi->irqn = map->irqn;
	sensor_instances[map->irq_channel] = hdi;

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = hdi->gpio_pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(hdi->gpio_port, &GPIO_InitStructure);

	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	uint8_t port_source = GPIO_PortSourceGPIOA;
	if (hdi->gpio_port == GPIOB)
		port_source = GPIO_PortSourceGPIOB;
	else if (hdi->gpio_port == GPIOC)
		port_source = GPIO_PortSourceGPIOC;
	else if (hdi->gpio_port == GPIOD)
		port_source = GPIO_PortSourceGPIOD;
	else if (hdi->gpio_port == GPIOE)
		port_source = GPIO_PortSourceGPIOE;

	GPIO_EXTILineConfig(port_source, GPIO_PinSource0 + DI_PinToSource(hdi->gpio_pin));

	EXTI_InitStructure.EXTI_Line = map->exti_line;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = map->irqn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);

	EXTI_ClearITPendingBit(map->exti_line);
}

static bool DI_Driver_Init(DI_handle_t hdi)
{
	if (hdi == NULL)
		return false;
	if (hdi->is_initialized)
		return true;

	DI_GPIO_Init(hdi);
	DI_InitEXTI(hdi);
	DI_EnableIRQ(hdi, false);
	DI_Read(hdi);
	hdi->is_initialized = true;
	return true;
}

static bool DI_Driver_Deinit(DI_handle_t hdi)
{
	if (hdi == NULL)
		return false;
	if (!hdi->is_initialized)
		return true;

	DI_EnableIRQ(hdi, false);
	if (hdi->irq_channel < EXTI_MAP_COUNT)
	{
		sensor_instances[hdi->irq_channel] = NULL;
	}
	hdi->is_initialized = false;
	return true;
}

static bool DI_Read(DI_handle_t hdi)
{
	if (hdi == NULL)
		return false;

	hdi->last_state = hdi->state;
	BitAction pin_state = GPIO_ReadInputDataBit(hdi->gpio_port, hdi->gpio_pin);
	hdi->state = (pin_state == Bit_SET);
	return hdi->state;
}

static bool DI_IsActive(DI_handle_t hdi)
{
	if (hdi == NULL)
		return false;

	DI_Read(hdi);
	return hdi->active_level ? hdi->state : !hdi->state;
}

static int DI_GetActiveState(DI_handle_t hdi)
{
	if (hdi == NULL)
		return -1;
	return hdi->active_level;
}

static void DI_EnableIRQ(DI_handle_t hdi, bool enable)
{
	if (hdi == NULL)
		return;

	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = hdi->exti_line;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = enable ? ENABLE : DISABLE;
	EXTI_Init(&EXTI_InitStructure);

	if (enable)
	{
		NVIC_EnableIRQ(hdi->irqn);
	}
	else
	{
		NVIC_DisableIRQ(hdi->irqn);
		EXTI_ClearITPendingBit(hdi->exti_line);
	}
}

static void DI_SetCallback(DI_handle_t hdi, DI_EventType event, DI_Callback callback, void *args)
{
	if (hdi == NULL)
		return;

	switch (event)
	{
	case DI_EVENT_RISING:
		hdi->rising_callback = callback;
		hdi->rising_args = args;
		break;
	case DI_EVENT_FALLING:
		hdi->falling_callback = callback;
		hdi->falling_args = args;
		break;
	case DI_EVENT_BOTH:
		hdi->both_callback = callback;
		hdi->both_args = args;
		break;
	default:
		break;
	}

	DI_EnableIRQ(hdi, true);
}

DI_handle_t DI_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, int active_level)
{
	DI_handle_t hdi = (DI_handle_t)malloc(sizeof(DI_dev_t));
	if (hdi == NULL)
	{
		return NULL;
	}

	hdi->gpio_port = gpio_port;
	hdi->gpio_pin = gpio_pin;
	hdi->active_level = active_level;
	hdi->state = false;
	hdi->last_state = false;
	hdi->rising_callback = NULL;
	hdi->falling_callback = NULL;
	hdi->both_callback = NULL;
	hdi->rising_args = NULL;
	hdi->falling_args = NULL;
	hdi->both_args = NULL;
	hdi->exti_line = 0;
	hdi->irq_channel = 0;
	hdi->irqn = 0;
	hdi->is_initialized = false;

	if (!DI_Driver_Init(hdi))
	{
		free(hdi);
		return NULL;
	}
	return hdi;
}

void DI_Destroy(DI_handle_t hdi)
{
	if (hdi == NULL)
		return;
	DI_Driver_Deinit(hdi);
	free(hdi);
}

DI_Operations *DI_GetOperations(void)
{
	return &di_ops;
}

static void DI_EXTI_IRQHandler(uint32_t exti_line)
{
	DI_handle_t hdi = NULL;
	for (int i = 0; i < EXTI_MAP_COUNT; i++)
	{
		if (sensor_instances[i] != NULL && sensor_instances[i]->exti_line == exti_line)
		{
			hdi = sensor_instances[i];
			break;
		}
	}

	if (hdi == NULL)
		return;

	hdi->last_state = hdi->state;
	hdi->state = GPIO_ReadInputDataBit(hdi->gpio_port, hdi->gpio_pin);

	if (!hdi->last_state && hdi->state)
	{
		if (hdi->rising_callback != NULL)
		{
			hdi->rising_callback(hdi, DI_EVENT_RISING, hdi->rising_args);
		}
		if (hdi->both_callback != NULL)
		{
			hdi->both_callback(hdi, DI_EVENT_RISING, hdi->both_args);
		}
	}
	else if (hdi->last_state && !hdi->state)
	{
		if (hdi->falling_callback != NULL)
		{
			hdi->falling_callback(hdi, DI_EVENT_FALLING, hdi->falling_args);
		}
		if (hdi->both_callback != NULL)
		{
			hdi->both_callback(hdi, DI_EVENT_FALLING, hdi->both_args);
		}
	}
}

// void EXTI0_IRQHandler(void)
// {
//     if (EXTI_GetITStatus(EXTI_Line0) != RESET)
//     {
//         DISensor_EXTI_IRQHandler(EXTI_Line0);
//         EXTI_ClearITPendingBit(EXTI_Line0); // 确保清除中断标志
//     }
// }

// void EXTI1_IRQHandler(void)
// {
//     if (EXTI_GetITStatus(EXTI_Line1) != RESET)
//     {
//         DISensor_EXTI_IRQHandler(EXTI_Line1);
//         EXTI_ClearITPendingBit(EXTI_Line1);
//     }
// }

// void EXTI2_IRQHandler(void)
// {
//     if (EXTI_GetITStatus(EXTI_Line2) != RESET)
//     {
//         DISensor_EXTI_IRQHandler(EXTI_Line2);
//         EXTI_ClearITPendingBit(EXTI_Line2);
//     }
// }

// void EXTI3_IRQHandler(void)
// {
//     if (EXTI_GetITStatus(EXTI_Line3) != RESET)
//     {
//         DISensor_EXTI_IRQHandler(EXTI_Line3);
//         EXTI_ClearITPendingBit(EXTI_Line3);
//     }
// }

// void EXTI4_IRQHandler(void)
//{
//	if (EXTI_GetITStatus(EXTI_Line4) != RESET)
//	{
//		DISensor_EXTI_IRQHandler(EXTI_Line4);
//		EXTI_ClearITPendingBit(EXTI_Line4);
//	}
//}

// void EXTI9_5_IRQHandler(void)
// {
// 	uint32_t exti_line = EXTI_Line5;
// 	while (exti_line <= EXTI_Line9)
// 	{
// 		if (EXTI_GetITStatus(exti_line) != RESET)
// 		{
// 			// 清除中断标志
// 			EXTI_ClearITPendingBit(exti_line);
// 			DI_EXTI_IRQHandler(exti_line);
// 		}
// 		exti_line <<= 1;
// 	}
// }

// void EXTI15_10_IRQHandler(void)
// {
// 	uint32_t exti_line = EXTI_Line10;
// 	while (exti_line <= EXTI_Line15)
// 	{
// 		if (EXTI_GetITStatus(exti_line) != RESET)
// 		{
// 			DISensor_EXTI_IRQHandler(exti_line);
// 		}
// 		exti_line <<= 1;
// 	}
// }