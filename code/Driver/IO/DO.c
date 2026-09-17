/* ============================================================
 * 模块     : DO 数字输出驱动
 * 功能     : 数字量输出的开关、翻转与有效电平适配
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "DO.h"
#include <stdlib.h>

static void DO_Open(DO_handle_t hdo);
static void DO_Close(DO_handle_t hdo);
static void DO_Toggle(DO_handle_t hdo);
static void DO_SetState(DO_handle_t hdo, bool state);
static void DO_SetEnable(DO_handle_t hdo, bool enable);
static bool DO_IsEnabled(DO_handle_t hdo);
static bool DO_IsOpen(DO_handle_t hdo);
static bool DO_Driver_Init(DO_handle_t hdo);
static bool DO_Driver_Deinit(DO_handle_t hdo);

static DO_Operations do_ops = {
	.init = DO_Driver_Init,
	.deinit = DO_Driver_Deinit,
	.create = DO_Create,
	.destroy = DO_Destroy,
	.open = DO_Open,
	.close = DO_Close,
	.toggle = DO_Toggle,
	.set_state = DO_SetState,
	.set_enable = DO_SetEnable,
	.is_enabled = DO_IsEnabled,
	.is_open = DO_IsOpen};

static void DO_GPIO_Init(DO_handle_t hdo)
{
	// 开启时钟
	if (hdo->gpio_port == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (hdo->gpio_port == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if (hdo->gpio_port == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if (hdo->gpio_port == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}

	GPIO_InitTypeDef GPIO_InitStructure;

	// 配置GPIO为推挽输出
	GPIO_InitStructure.GPIO_Pin = hdo->gpio_pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(hdo->gpio_port, &GPIO_InitStructure);
}

static bool DO_Driver_Init(DO_handle_t hdo)
{
	if (hdo == NULL)
		return false;
	if (hdo->is_initialized)
		return true;

	DO_GPIO_Init(hdo);
	DO_Close(hdo);
	hdo->is_initialized = true;
	return true;
}

static bool DO_Driver_Deinit(DO_handle_t hdo)
{
	if (hdo == NULL)
		return false;
	if (!hdo->is_initialized)
		return true;

	DO_Close(hdo);
	hdo->is_initialized = false;
	return true;
}

static void DO_Open(DO_handle_t hdo)
{
	if (hdo == NULL)
		return;

	hdo->state = true;
	if (hdo->enabled)
	{
		GPIO_WriteBit(hdo->gpio_port, hdo->gpio_pin, hdo->active_level ? Bit_SET : Bit_RESET);
	}
}

static void DO_Close(DO_handle_t hdo)
{
	if (hdo == NULL)
		return;

	hdo->state = false;
	if (hdo->enabled)
	{
		GPIO_WriteBit(hdo->gpio_port, hdo->gpio_pin, hdo->active_level ? Bit_RESET : Bit_SET);
	}
}

static void DO_Toggle(DO_handle_t hdo)
{
	if (hdo == NULL)
		return;
	DO_SetState(hdo, !hdo->state);
}

static void DO_SetState(DO_handle_t hdo, bool state)
{
	if (hdo == NULL)
		return;
	if (state)
	{
		DO_Open(hdo);
	}
	else
	{
		DO_Close(hdo);
	}
}

static void DO_SetEnable(DO_handle_t hdo, bool enable)
{
	if (hdo == NULL)
		return;

	hdo->enabled = enable;
	if (!enable)
	{
		GPIO_WriteBit(hdo->gpio_port, hdo->gpio_pin, hdo->active_level ? Bit_RESET : Bit_SET);
		return;
	}
	DO_SetState(hdo, hdo->state);
}

static bool DO_IsEnabled(DO_handle_t hdo)
{
	if (hdo == NULL)
		return false;
	return hdo->enabled;
}

static bool DO_IsOpen(DO_handle_t hdo)
{
	if (hdo == NULL)
		return false;
	return hdo->state;
}

DO_handle_t DO_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, int active_level)
{
	DO_handle_t hdo = (DO_handle_t)malloc(sizeof(DO_dev_t));
	if (hdo == NULL)
		return NULL;

	hdo->gpio_port = gpio_port;
	hdo->gpio_pin = gpio_pin;
	hdo->active_level = active_level;
	hdo->state = false;
	hdo->enabled = true;
	hdo->is_initialized = false;

	if (!DO_Driver_Init(hdo))
	{
		free(hdo);
		return NULL;
	}
	return hdo;
}

void DO_Destroy(DO_handle_t hdo)
{
	if (hdo == NULL)
		return;
	DO_Driver_Deinit(hdo);
	free(hdo);
}

DO_Operations *DO_GetOperations(void)
{
	return &do_ops;
}
