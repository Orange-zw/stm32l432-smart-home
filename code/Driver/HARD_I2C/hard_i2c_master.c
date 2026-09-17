#include "hard_i2c_master.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#define HARD_I2C_BUS_POOL_SIZE (2U)
#define HARD_I2C_TIMEOUT_TICK  (60000U)

struct hard_i2c_master_bus_impl_t
{
	uint8_t     in_use;
	I2C_TypeDef *instance;
	uint32_t    clock_hz;
};

static struct hard_i2c_master_bus_impl_t g_hard_i2c_bus_pool[HARD_I2C_BUS_POOL_SIZE];

static hard_i2c_err_t hard_i2c_hw_init(I2C_TypeDef *instance, uint32_t clock_hz, uint8_t remap_enable);
static hard_i2c_err_t hard_i2c_start(I2C_TypeDef *instance);
static hard_i2c_err_t hard_i2c_send_addr(I2C_TypeDef *instance, uint8_t addr_7bit, uint8_t direction);
static hard_i2c_err_t hard_i2c_write_bytes(I2C_TypeDef *instance, const uint8_t *data, uint16_t len);
static hard_i2c_err_t hard_i2c_read_bytes(I2C_TypeDef *instance, uint8_t *data, uint16_t len);
static hard_i2c_err_t hard_i2c_wait_event(I2C_TypeDef *instance, uint32_t event);
static uint32_t       hard_i2c_freq_to_hz(hard_i2c_master_freq_t freq);

hard_i2c_err_t hard_i2c_master_new(const hard_i2c_master_config_t *config, hard_i2c_master_bus_t *bus)
{
	struct hard_i2c_master_bus_impl_t *bus_impl = NULL;
	uint8_t                            i;
	uint32_t                           hz;
	hard_i2c_err_t                     ret;

	if (config == NULL || bus == NULL || config->instance == NULL || config->freq >= HARD_I2C_FREQ_END)
	{
		return HARD_I2C_ERR_INVALID_ARG;
	}

	if (config->instance != I2C1 && config->instance != I2C2)
	{
		return HARD_I2C_ERR_INVALID_ARG;
	}

	for (i = 0U; i < HARD_I2C_BUS_POOL_SIZE; i++)
	{
		if (g_hard_i2c_bus_pool[i].in_use != 0U && g_hard_i2c_bus_pool[i].instance == config->instance)
		{
			return HARD_I2C_ERR_FAIL;
		}
	}

	for (i = 0U; i < HARD_I2C_BUS_POOL_SIZE; i++)
	{
		if (g_hard_i2c_bus_pool[i].in_use == 0U)
		{
			bus_impl = &g_hard_i2c_bus_pool[i];
			bus_impl->in_use = 1U;
			break;
		}
	}

	if (bus_impl == NULL)
	{
		return HARD_I2C_ERR_NO_MEM;
	}

	hz = hard_i2c_freq_to_hz(config->freq);
	ret = hard_i2c_hw_init(config->instance, hz, config->remap_enable);
	if (ret != HARD_I2C_OK)
	{
		bus_impl->in_use = 0U;
		return ret;
	}

	bus_impl->instance = config->instance;
	bus_impl->clock_hz = hz;
	*bus = bus_impl;
	return HARD_I2C_OK;
}

hard_i2c_err_t hard_i2c_master_del(hard_i2c_master_bus_t bus)
{
	if (bus == NULL)
	{
		return HARD_I2C_ERR_INVALID_ARG;
	}

	I2C_Cmd(bus->instance, DISABLE);
	bus->instance = NULL;
	bus->clock_hz = 0U;
	bus->in_use = 0U;
	return HARD_I2C_OK;
}

hard_i2c_err_t hard_i2c_master_write(
    hard_i2c_master_bus_t bus,
    uint8_t               device_address,
    const uint8_t        *write_buffer,
    uint16_t              write_size)
{
	hard_i2c_err_t ret;

	if (bus == NULL || write_buffer == NULL || write_size == 0U || device_address >= 0x80U)
	{
		return HARD_I2C_ERR_INVALID_ARG;
	}

	ret = hard_i2c_start(bus->instance);
	if (ret != HARD_I2C_OK)
	{
		return ret;
	}

	ret = hard_i2c_send_addr(bus->instance, device_address, I2C_Direction_Transmitter);
	if (ret != HARD_I2C_OK)
	{
		I2C_GenerateSTOP(bus->instance, ENABLE);
		return ret;
	}

	ret = hard_i2c_write_bytes(bus->instance, write_buffer, write_size);
	I2C_GenerateSTOP(bus->instance, ENABLE);
	return ret;
}

hard_i2c_err_t hard_i2c_master_read(
    hard_i2c_master_bus_t bus,
    uint8_t               device_address,
    uint8_t              *read_buffer,
    uint16_t              read_size)
{
	hard_i2c_err_t ret;

	if (bus == NULL || read_buffer == NULL || read_size == 0U || device_address >= 0x80U)
	{
		return HARD_I2C_ERR_INVALID_ARG;
	}

	ret = hard_i2c_start(bus->instance);
	if (ret != HARD_I2C_OK)
	{
		return ret;
	}

	ret = hard_i2c_send_addr(bus->instance, device_address, I2C_Direction_Receiver);
	if (ret != HARD_I2C_OK)
	{
		I2C_GenerateSTOP(bus->instance, ENABLE);
		return ret;
	}

	ret = hard_i2c_read_bytes(bus->instance, read_buffer, read_size);
	return ret;
}

hard_i2c_err_t hard_i2c_master_write_read(
    hard_i2c_master_bus_t bus,
    uint8_t               device_address,
    const uint8_t        *write_buffer,
    uint16_t              write_size,
    uint8_t              *read_buffer,
    uint16_t              read_size)
{
	hard_i2c_err_t ret;

	if (bus == NULL || write_buffer == NULL || write_size == 0U ||
	    read_buffer == NULL || read_size == 0U || device_address >= 0x80U)
	{
		return HARD_I2C_ERR_INVALID_ARG;
	}

	ret = hard_i2c_start(bus->instance);
	if (ret != HARD_I2C_OK)
	{
		return ret;
	}

	ret = hard_i2c_send_addr(bus->instance, device_address, I2C_Direction_Transmitter);
	if (ret != HARD_I2C_OK)
	{
		I2C_GenerateSTOP(bus->instance, ENABLE);
		return ret;
	}

	ret = hard_i2c_write_bytes(bus->instance, write_buffer, write_size);
	if (ret != HARD_I2C_OK)
	{
		I2C_GenerateSTOP(bus->instance, ENABLE);
		return ret;
	}

	/* ReSTART: 不发STOP，直接再次START进入读阶段 */
	ret = hard_i2c_start(bus->instance);
	if (ret != HARD_I2C_OK)
	{
		I2C_GenerateSTOP(bus->instance, ENABLE);
		return ret;
	}

	ret = hard_i2c_send_addr(bus->instance, device_address, I2C_Direction_Receiver);
	if (ret != HARD_I2C_OK)
	{
		I2C_GenerateSTOP(bus->instance, ENABLE);
		return ret;
	}

	ret = hard_i2c_read_bytes(bus->instance, read_buffer, read_size);
	return ret;
}

static hard_i2c_err_t hard_i2c_hw_init(I2C_TypeDef *instance, uint32_t clock_hz, uint8_t remap_enable)
{
	GPIO_InitTypeDef gpio_init;
	I2C_InitTypeDef  i2c_init;

	/* 仅根据 I2C 外设号自动选用默认引脚:
	 * I2C1 -> PB6/PB7（不重映射）或 PB8/PB9（重映射）
	 * I2C2 -> PB10/PB11
	 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	if (instance == I2C1)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
		if (remap_enable != 0U)
		{
			GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
			gpio_init.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
		}
		else
		{
			GPIO_PinRemapConfig(GPIO_Remap_I2C1, DISABLE);
			gpio_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
		}
	}
	else
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
		gpio_init.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	}

	gpio_init.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio_init);

	I2C_DeInit(instance);
	I2C_Cmd(instance, DISABLE);

	i2c_init.I2C_ClockSpeed = clock_hz;
	i2c_init.I2C_Mode = I2C_Mode_I2C;
	i2c_init.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c_init.I2C_OwnAddress1 = 0x00;
	i2c_init.I2C_Ack = I2C_Ack_Enable;
	i2c_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init(instance, &i2c_init);

	I2C_AcknowledgeConfig(instance, ENABLE);
	I2C_Cmd(instance, ENABLE);
	return HARD_I2C_OK;
}

static hard_i2c_err_t hard_i2c_start(I2C_TypeDef *instance)
{
	uint32_t timeout = HARD_I2C_TIMEOUT_TICK;

	while (I2C_GetFlagStatus(instance, I2C_FLAG_BUSY) == SET)
	{
		if (timeout-- == 0U)
		{
			return HARD_I2C_ERR_TIMEOUT;
		}
	}

	I2C_GenerateSTART(instance, ENABLE);
	return hard_i2c_wait_event(instance, I2C_EVENT_MASTER_MODE_SELECT);
}

static hard_i2c_err_t hard_i2c_send_addr(I2C_TypeDef *instance, uint8_t addr_7bit, uint8_t direction)
{
	uint32_t timeout = HARD_I2C_TIMEOUT_TICK;

	I2C_Send7bitAddress(instance, (uint8_t)(addr_7bit << 1), direction);

	while (I2C_GetFlagStatus(instance, I2C_FLAG_ADDR) == RESET)
	{
		if (I2C_GetFlagStatus(instance, I2C_FLAG_AF) == SET)
		{
			I2C_ClearFlag(instance, I2C_FLAG_AF);
			return HARD_I2C_ERR_NOT_FOUND;
		}
		if (timeout-- == 0U)
		{
			return HARD_I2C_ERR_TIMEOUT;
		}
	}

	/* 读取 SR1/SR2 清除 ADDR 标志 */
	(void)instance->SR1;
	(void)instance->SR2;

	if (direction == I2C_Direction_Transmitter)
	{
		return hard_i2c_wait_event(instance, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
	}
	return hard_i2c_wait_event(instance, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
}

static hard_i2c_err_t hard_i2c_write_bytes(I2C_TypeDef *instance, const uint8_t *data, uint16_t len)
{
	uint16_t i;

	for (i = 0U; i < len; i++)
	{
		I2C_SendData(instance, data[i]);
		if (hard_i2c_wait_event(instance, I2C_EVENT_MASTER_BYTE_TRANSMITTED) != HARD_I2C_OK)
		{
			return HARD_I2C_ERR_TIMEOUT;
		}
	}
	return HARD_I2C_OK;
}

static hard_i2c_err_t hard_i2c_read_bytes(I2C_TypeDef *instance, uint8_t *data, uint16_t len)
{
	uint16_t i;
	uint32_t timeout;

	if (len == 1U)
	{
		I2C_AcknowledgeConfig(instance, DISABLE);
		I2C_GenerateSTOP(instance, ENABLE);

		timeout = HARD_I2C_TIMEOUT_TICK;
		while (I2C_GetFlagStatus(instance, I2C_FLAG_RXNE) == RESET)
		{
			if (timeout-- == 0U)
			{
				I2C_AcknowledgeConfig(instance, ENABLE);
				return HARD_I2C_ERR_TIMEOUT;
			}
		}
		data[0] = I2C_ReceiveData(instance);
		I2C_AcknowledgeConfig(instance, ENABLE);
		return HARD_I2C_OK;
	}

	for (i = 0U; i < len; i++)
	{
		if (i == (uint16_t)(len - 1U))
		{
			I2C_AcknowledgeConfig(instance, DISABLE);
			I2C_GenerateSTOP(instance, ENABLE);
		}

		timeout = HARD_I2C_TIMEOUT_TICK;
		while (I2C_GetFlagStatus(instance, I2C_FLAG_RXNE) == RESET)
		{
			if (timeout-- == 0U)
			{
				I2C_AcknowledgeConfig(instance, ENABLE);
				return HARD_I2C_ERR_TIMEOUT;
			}
		}
		data[i] = I2C_ReceiveData(instance);
	}

	I2C_AcknowledgeConfig(instance, ENABLE);
	return HARD_I2C_OK;
}

static hard_i2c_err_t hard_i2c_wait_event(I2C_TypeDef *instance, uint32_t event)
{
	uint32_t timeout = HARD_I2C_TIMEOUT_TICK;

	while (I2C_CheckEvent(instance, event) == ERROR)
	{
		if (timeout-- == 0U)
		{
			return HARD_I2C_ERR_TIMEOUT;
		}
	}
	return HARD_I2C_OK;
}

static uint32_t hard_i2c_freq_to_hz(hard_i2c_master_freq_t freq)
{
	switch (freq)
	{
		case HARD_I2C_100KHZ:
			return 100000U;
		case HARD_I2C_400KHZ:
			return 400000U;
		default:
			return 100000U;
	}
}
