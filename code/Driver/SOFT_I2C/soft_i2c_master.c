/* ============================================================
 * 模块     : 软件 I2C 主机驱动
 * 功能     : GPIO 位翻转模拟 I2C 时序，用于 OLED 等器件
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "soft_i2c_master.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#define SOFT_I2C_GPIO_SET_HIGH(_port, _pin_mask) ((_port)->BSRR = (_pin_mask))
#define SOFT_I2C_GPIO_SET_LOW(_port, _pin_mask) ((_port)->BRR = (_pin_mask))
#define SOFT_I2C_GPIO_READ(_port, _pin_mask) (((_port)->IDR & (_pin_mask)) ? 1U : 0U)

struct i2c_master_bus_impl_t
{
	uint8_t       in_use;
	GPIO_TypeDef *scl_port;
	uint16_t      scl_pin;
	GPIO_TypeDef *sda_port;
	uint16_t      sda_pin;
	uint16_t      delay_us;
};

#define SOFT_I2C_BUS_POOL_SIZE (2U)
static struct i2c_master_bus_impl_t g_soft_i2c_bus_pool[SOFT_I2C_BUS_POOL_SIZE];

static soft_i2c_err_t soft_i2c_enable_gpio_clock(GPIO_TypeDef *port);
static uint16_t       soft_i2c_freq_to_delay(soft_i2c_master_freq_t freq);
static void           soft_i2c_line_set_scl(soft_i2c_master_bus_t bus, uint8_t level);
static void           soft_i2c_line_set_sda(soft_i2c_master_bus_t bus, uint8_t level);
static uint8_t        soft_i2c_line_get_sda(soft_i2c_master_bus_t bus);
static void           soft_i2c_start(soft_i2c_master_bus_t bus);
static void           soft_i2c_stop(soft_i2c_master_bus_t bus);
static uint8_t        soft_i2c_write_byte(soft_i2c_master_bus_t bus, uint8_t byte);
static uint8_t        soft_i2c_read_byte(soft_i2c_master_bus_t bus, uint8_t send_ack);
static soft_i2c_err_t soft_i2c_transfer(
    soft_i2c_master_bus_t bus,
    uint8_t               device_address,
    const uint8_t        *write_buffer,
    uint16_t              write_size,
    uint8_t              *read_buffer,
    uint16_t              read_size);

static void soft_i2c_delay_us(uint32_t us)
{
	extern void delay_us(uint32_t us);
	delay_us(us);
}

soft_i2c_err_t soft_i2c_master_new(const soft_i2c_master_config_t *config, soft_i2c_master_bus_t *bus)
{
	GPIO_InitTypeDef              gpio_init;
	struct i2c_master_bus_impl_t *bus_impl;
	soft_i2c_err_t                ret;
	uint8_t                       i;

	if (config == NULL || bus == NULL)
	{
		return SOFT_I2C_ERR_INVALID_ARG;
	}

	if (config->freq >= SOFT_I2C_FREQ_END ||
	    config->scl_port == NULL || config->sda_port == NULL ||
	    config->scl_pin == 0U || config->sda_pin == 0U)
	{
		return SOFT_I2C_ERR_INVALID_ARG;
	}

	ret = soft_i2c_enable_gpio_clock(config->scl_port);
	if (ret != SOFT_I2C_OK)
	{
		return ret;
	}

	ret = soft_i2c_enable_gpio_clock(config->sda_port);
	if (ret != SOFT_I2C_OK)
	{
		return ret;
	}

	bus_impl = NULL;
	for (i = 0U; i < SOFT_I2C_BUS_POOL_SIZE; i++)
	{
		if (g_soft_i2c_bus_pool[i].in_use == 0U)
		{
			bus_impl = &g_soft_i2c_bus_pool[i];
			bus_impl->in_use = 1U;
			break;
		}
	}

	if (bus_impl == NULL)
	{
		return SOFT_I2C_ERR_NO_MEM;
	}

	bus_impl->scl_port = config->scl_port;
	bus_impl->scl_pin = config->scl_pin;
	bus_impl->sda_port = config->sda_port;
	bus_impl->sda_pin = config->sda_pin;
	bus_impl->delay_us = soft_i2c_freq_to_delay(config->freq);

	/* SCL/SDA 使用开漏输出，写1表示释放总线，由上拉电阻拉高 */
	gpio_init.GPIO_Pin = config->scl_pin;
	gpio_init.GPIO_Mode = GPIO_Mode_Out_OD;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(config->scl_port, &gpio_init);

	gpio_init.GPIO_Pin = config->sda_pin;
	GPIO_Init(config->sda_port, &gpio_init);

	/* 空闲态：SCL=1, SDA=1 */
	SOFT_I2C_GPIO_SET_HIGH(config->scl_port, config->scl_pin);
	SOFT_I2C_GPIO_SET_HIGH(config->sda_port, config->sda_pin);

	*bus = bus_impl;
	return SOFT_I2C_OK;
}

soft_i2c_err_t soft_i2c_master_del(soft_i2c_master_bus_t bus)
{
	if (bus == NULL)
	{
		return SOFT_I2C_ERR_INVALID_ARG;
	}

	/* 释放前把总线恢复到空闲态 */
	SOFT_I2C_GPIO_SET_HIGH(bus->scl_port, bus->scl_pin);
	SOFT_I2C_GPIO_SET_HIGH(bus->sda_port, bus->sda_pin);
	bus->in_use = 0U;
	return SOFT_I2C_OK;
}

soft_i2c_err_t soft_i2c_master_write(
    soft_i2c_master_bus_t bus,
    uint8_t               device_address,
    const uint8_t        *write_buffer,
    uint16_t              write_size)
{
	if (bus == NULL || device_address >= 0x80U || write_buffer == NULL || write_size == 0U)
	{
		return SOFT_I2C_ERR_INVALID_ARG;
	}

	return soft_i2c_transfer(bus, device_address, write_buffer, write_size, NULL, 0U);
}

soft_i2c_err_t soft_i2c_master_read(
    soft_i2c_master_bus_t bus,
    uint8_t               device_address,
    uint8_t              *read_buffer,
    uint16_t              read_size)
{
	if (bus == NULL || device_address >= 0x80U || read_buffer == NULL || read_size == 0U)
	{
		return SOFT_I2C_ERR_INVALID_ARG;
	}

	return soft_i2c_transfer(bus, device_address, NULL, 0U, read_buffer, read_size);
}

soft_i2c_err_t soft_i2c_master_write_read(
    soft_i2c_master_bus_t bus,
    uint8_t               device_address,
    const uint8_t        *write_buffer,
    uint16_t              write_size,
    uint8_t              *read_buffer,
    uint16_t              read_size)
{
	if (bus == NULL || device_address >= 0x80U ||
	    write_buffer == NULL || write_size == 0U ||
	    read_buffer == NULL || read_size == 0U)
	{
		return SOFT_I2C_ERR_INVALID_ARG;
	}

	return soft_i2c_transfer(bus, device_address, write_buffer, write_size, read_buffer, read_size);
}

static soft_i2c_err_t soft_i2c_enable_gpio_clock(GPIO_TypeDef *port)
{
	if (port == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (port == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if (port == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if (port == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}
	else if (port == GPIOE)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	}
#ifdef RCC_APB2Periph_GPIOF
	else if (port == GPIOF)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	}
#endif
#ifdef RCC_APB2Periph_GPIOG
	else if (port == GPIOG)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
	}
#endif
	else
	{
		return SOFT_I2C_ERR_INVALID_ARG;
	}
	return SOFT_I2C_OK;
}

static uint16_t soft_i2c_freq_to_delay(soft_i2c_master_freq_t freq)
{
	switch (freq)
	{
	case SOFT_I2C_100KHZ:
		/* 保守值，保证更多传感器兼容性 */
		return 5U;
	case SOFT_I2C_200KHZ:
		return 2U;
	case SOFT_I2C_300KHZ:
		return 1U;
	default:
		return 5U;
	}
}

static void soft_i2c_line_set_scl(soft_i2c_master_bus_t bus, uint8_t level)
{
	if (level != 0U)
	{
		SOFT_I2C_GPIO_SET_HIGH(bus->scl_port, bus->scl_pin);
	}
	else
	{
		SOFT_I2C_GPIO_SET_LOW(bus->scl_port, bus->scl_pin);
	}
	soft_i2c_delay_us(bus->delay_us);
}

static void soft_i2c_line_set_sda(soft_i2c_master_bus_t bus, uint8_t level)
{
	if (level != 0U)
	{
		SOFT_I2C_GPIO_SET_HIGH(bus->sda_port, bus->sda_pin);
	}
	else
	{
		SOFT_I2C_GPIO_SET_LOW(bus->sda_port, bus->sda_pin);
	}
	soft_i2c_delay_us(bus->delay_us);
}

static uint8_t soft_i2c_line_get_sda(soft_i2c_master_bus_t bus)
{
	return (uint8_t)SOFT_I2C_GPIO_READ(bus->sda_port, bus->sda_pin);
}

static void soft_i2c_start(soft_i2c_master_bus_t bus)
{
	/* START 条件：SCL=1 时，SDA 从 1 拉到 0 */
	soft_i2c_line_set_scl(bus, 1U);
	soft_i2c_line_set_sda(bus, 1U);
	soft_i2c_line_set_sda(bus, 0U);
}

static void soft_i2c_stop(soft_i2c_master_bus_t bus)
{
	/* STOP 条件：SCL=1 时，SDA 从 0 释放为 1 */
	soft_i2c_line_set_scl(bus, 0U);
	soft_i2c_line_set_sda(bus, 0U);
	soft_i2c_line_set_scl(bus, 1U);
	soft_i2c_line_set_sda(bus, 1U);
}

static uint8_t soft_i2c_write_byte(soft_i2c_master_bus_t bus, uint8_t byte)
{
	uint8_t bit_val;
	int     i;

	for (i = 7; i >= 0; i--)
	{
		bit_val = (uint8_t)((byte >> i) & 0x01U);
		soft_i2c_line_set_scl(bus, 0U);
		soft_i2c_line_set_sda(bus, bit_val);
		soft_i2c_line_set_scl(bus, 1U);
	}

	/* 第9个时钟读取ACK：主机释放SDA，等待从机下拉 */
	soft_i2c_line_set_scl(bus, 0U);
	soft_i2c_line_set_sda(bus, 1U);
	soft_i2c_line_set_scl(bus, 1U);

	bit_val = (uint8_t)(soft_i2c_line_get_sda(bus) == 0U ? 1U : 0U);

	/* 时钟拉低，结束ACK采样窗口 */
	soft_i2c_line_set_scl(bus, 0U);
	return bit_val;
}

static uint8_t soft_i2c_read_byte(soft_i2c_master_bus_t bus, uint8_t send_ack)
{
	uint8_t result;
	uint8_t bit_in;
	uint8_t i;

	result = 0U;

	/* 读数据前释放SDA，让从机驱动数据线 */
	soft_i2c_line_set_scl(bus, 0U);
	soft_i2c_line_set_sda(bus, 1U);

	for (i = 0U; i < 8U; i++)
	{
		soft_i2c_line_set_scl(bus, 0U);
		soft_i2c_line_set_scl(bus, 1U);
		/* I2C在SCL高电平期间采样数据位，降低边沿抖动导致的误码概率 */
		bit_in = soft_i2c_line_get_sda(bus);
		result = (uint8_t)((result << 1) | (bit_in & 0x01U));
	}

	/* 第9位由主机返回ACK/NACK：ACK=0, NACK=1 */
	soft_i2c_line_set_scl(bus, 0U);
	soft_i2c_line_set_sda(bus, (uint8_t)(send_ack ? 0U : 1U));
	soft_i2c_line_set_scl(bus, 1U);
	soft_i2c_line_set_scl(bus, 0U);
	soft_i2c_line_set_sda(bus, 1U);

	return result;
}

static soft_i2c_err_t soft_i2c_transfer(
    soft_i2c_master_bus_t bus,
    uint8_t               device_address,
    const uint8_t        *write_buffer,
    uint16_t              write_size,
    uint8_t              *read_buffer,
    uint16_t              read_size)
{
	soft_i2c_err_t ret;
	uint8_t        ack;
	uint16_t       i;

	ret = SOFT_I2C_OK;

	/* 进入传输前先释放总线，尽量保证在已知状态开始 */
	SOFT_I2C_GPIO_SET_HIGH(bus->scl_port, bus->scl_pin);
	SOFT_I2C_GPIO_SET_HIGH(bus->sda_port, bus->sda_pin);

	if (write_buffer != NULL && write_size != 0U)
	{
		soft_i2c_start(bus);
		ack = soft_i2c_write_byte(bus, (uint8_t)(device_address << 1));
		if (ack == 0U)
		{
			ret = SOFT_I2C_ERR_NOT_FOUND;
		}
		else
		{
			for (i = 0U; i < write_size; i++)
			{
				ack = soft_i2c_write_byte(bus, write_buffer[i]);
				if (ack == 0U)
				{
					ret = SOFT_I2C_ERR_FAIL;
					break;
				}
			}
		}
	}

	if (ret == SOFT_I2C_OK && read_buffer != NULL && read_size != 0U)
	{
		soft_i2c_start(bus);
		ack = soft_i2c_write_byte(bus, (uint8_t)((device_address << 1) | 0x01U));
		if (ack == 0U)
		{
			ret = SOFT_I2C_ERR_NOT_FOUND;
		}
		else
		{
			for (i = 0U; i < read_size; i++)
			{
				/* 最后一个字节返回NACK，通知从机结束读取 */
				read_buffer[i] = soft_i2c_read_byte(bus, (uint8_t)(i != (read_size - 1U)));
			}
		}
	}

	soft_i2c_stop(bus);
	return ret;
}
