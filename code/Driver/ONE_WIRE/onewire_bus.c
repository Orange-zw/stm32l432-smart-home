/* ============================================================
 * 模块     : 单总线驱动
 * 功能     : 单总线时序与设备搜索，供 DS18B20 等器件使用
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "onewire_bus.h"
#include "stdlib.h"
#include "string.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#define ONEWIRE_GPIO_SET_HIGH(_port, _pin_mask) ((_port)->BSRR = (_pin_mask))
#define ONEWIRE_GPIO_SET_LOW(_port, _pin_mask) ((_port)->BRR = (_pin_mask))
#define ONEWIRE_GPIO_READ(_port, _pin_mask) (((_port)->IDR & (_pin_mask)) ? 1U : 0U)

#define ONEWIRE_RESET_PULSE_US (500U)
#define ONEWIRE_RESET_WAIT_US (200U)
#define ONEWIRE_RESET_PRESENCE_WAIT_MIN (15U)
#define ONEWIRE_RESET_PRESENCE_LOW_MIN (60U)

#define ONEWIRE_SLOT_START_US (2U)
#define ONEWIRE_SLOT_BIT_US (60U)
#define ONEWIRE_SLOT_RECOVERY_US (5U)
#define ONEWIRE_SLOT_SAMPLE_US (15U)

struct onewire_bus_t
{
	GPIO_TypeDef *dq_port;
	uint16_t      dq_pin;
	uint8_t       pin_index;
	uint8_t       en_pull_up;
};

struct onewire_device_iter_t
{
	onewire_bus_handle_t bus;
	uint16_t             last_discrepancy;
	uint8_t              is_last_device;
	uint8_t              rom_number[8];
};

static onewire_err_t onewire_enable_gpio_clock(GPIO_TypeDef *port);
static uint8_t       onewire_pin_to_index(uint16_t pin);
static void          onewire_pin_mode_output_pp(onewire_bus_handle_t bus);
static void          onewire_pin_mode_input_float(onewire_bus_handle_t bus);
static void          onewire_pin_mode_input_pu(onewire_bus_handle_t bus);
static void          onewire_bus_release(onewire_bus_handle_t bus);
static uint32_t      onewire_enter_critical(void);
static void          onewire_exit_critical(uint32_t primask);
static void          onewire_delay_us(uint32_t us);

static onewire_err_t onewire_enable_gpio_clock(GPIO_TypeDef *port)
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
		return ONEWIRE_ERR_INVALID_ARG;
	}
	return ONEWIRE_OK;
}

static uint8_t onewire_pin_to_index(uint16_t pin)
{
	uint8_t idx;
	for (idx = 0U; idx < 16U; idx++)
	{
		if (pin == ((uint16_t)1U << idx))
		{
			return idx;
		}
	}
	return 0xFFU;
}

static void onewire_pin_mode_output_pp(onewire_bus_handle_t bus)
{
	uint32_t shift;
	if (bus->pin_index < 8U)
	{
		shift = (uint32_t)bus->pin_index * 4U;
		bus->dq_port->CRL &= ~(0x0FU << shift);
		bus->dq_port->CRL |= (0x03U << shift);
	}
	else
	{
		shift = (uint32_t)(bus->pin_index - 8U) * 4U;
		bus->dq_port->CRH &= ~(0x0FU << shift);
		bus->dq_port->CRH |= (0x03U << shift);
	}
}

static void onewire_pin_mode_input_float(onewire_bus_handle_t bus)
{
	uint32_t shift;
	if (bus->pin_index < 8U)
	{
		shift = (uint32_t)bus->pin_index * 4U;
		bus->dq_port->CRL &= ~(0x0FU << shift);
		bus->dq_port->CRL |= (0x04U << shift);
	}
	else
	{
		shift = (uint32_t)(bus->pin_index - 8U) * 4U;
		bus->dq_port->CRH &= ~(0x0FU << shift);
		bus->dq_port->CRH |= (0x04U << shift);
	}
}

static void onewire_pin_mode_input_pu(onewire_bus_handle_t bus)
{
	uint32_t shift;
	ONEWIRE_GPIO_SET_HIGH(bus->dq_port, bus->dq_pin);
	if (bus->pin_index < 8U)
	{
		shift = (uint32_t)bus->pin_index * 4U;
		bus->dq_port->CRL &= ~(0x0FU << shift);
		bus->dq_port->CRL |= (0x08U << shift);
	}
	else
	{
		shift = (uint32_t)(bus->pin_index - 8U) * 4U;
		bus->dq_port->CRH &= ~(0x0FU << shift);
		bus->dq_port->CRH |= (0x08U << shift);
	}
}

static void onewire_bus_release(onewire_bus_handle_t bus)
{
	if (bus->en_pull_up != 0U)
	{
		onewire_pin_mode_input_pu(bus);
	}
	else
	{
		onewire_pin_mode_input_float(bus);
	}
}

static uint32_t onewire_enter_critical(void)
{
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	return primask;
}

static void onewire_exit_critical(uint32_t primask)
{
	if ((primask & 0x1U) == 0U)
	{
		__enable_irq();
	}
}

static void onewire_delay_us(uint32_t us)
{
	extern void delay_us(uint32_t us);
	delay_us(us);
}

onewire_err_t onewire_new_bus_gpio(const onewire_bus_config_t *bus_config, onewire_bus_handle_t *ret_bus)
{
	struct onewire_bus_t *bus_obj;
	uint8_t               pin_index;
	onewire_err_t         ret;

	if (bus_config == NULL || ret_bus == NULL || bus_config->dq_port == NULL || bus_config->dq_pin == 0U)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	pin_index = onewire_pin_to_index(bus_config->dq_pin);
	if (pin_index == 0xFFU)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	ret = onewire_enable_gpio_clock(bus_config->dq_port);
	if (ret != ONEWIRE_OK)
	{
		return ret;
	}

	bus_obj = (struct onewire_bus_t *)malloc(sizeof(struct onewire_bus_t));
	if (bus_obj == NULL)
	{
		return ONEWIRE_ERR_NO_MEM;
	}

	bus_obj->dq_port = bus_config->dq_port;
	bus_obj->dq_pin = bus_config->dq_pin;
	bus_obj->pin_index = pin_index;
	bus_obj->en_pull_up = (bus_config->flags.en_pull_up != 0U) ? 1U : 0U;

	onewire_bus_release(bus_obj);
	*ret_bus = bus_obj;
	return ONEWIRE_OK;
}

onewire_err_t onewire_bus_del(onewire_bus_handle_t bus)
{
	if (bus == NULL)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	onewire_bus_release(bus);
	free(bus);
	return ONEWIRE_OK;
}

onewire_err_t onewire_bus_reset(onewire_bus_handle_t bus)
{
	uint32_t key;
	uint8_t  presence = 0U;
	uint16_t wait_us;
	uint8_t  line_level;

	if (bus == NULL)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	key = onewire_enter_critical();

	onewire_pin_mode_output_pp(bus);
	ONEWIRE_GPIO_SET_LOW(bus->dq_port, bus->dq_pin);
	onewire_delay_us(ONEWIRE_RESET_PULSE_US);

	onewire_bus_release(bus);
	onewire_delay_us(ONEWIRE_RESET_PRESENCE_WAIT_MIN);

	wait_us = 0U;
	while (wait_us < ONEWIRE_RESET_WAIT_US)
	{
		line_level = (uint8_t)ONEWIRE_GPIO_READ(bus->dq_port, bus->dq_pin);
		if (line_level == 0U)
		{
			presence = 1U;
			break;
		}
		onewire_delay_us(1U);
		wait_us++;
	}

	if (presence != 0U)
	{
		wait_us = 0U;
		while (wait_us < (ONEWIRE_RESET_WAIT_US + ONEWIRE_RESET_PRESENCE_LOW_MIN))
		{
			line_level = (uint8_t)ONEWIRE_GPIO_READ(bus->dq_port, bus->dq_pin);
			if (line_level != 0U)
			{
				break;
			}
			onewire_delay_us(1U);
			wait_us++;
		}
	}

	onewire_delay_us(ONEWIRE_RESET_WAIT_US);
	onewire_exit_critical(key);

	return (presence != 0U) ? ONEWIRE_OK : ONEWIRE_ERR_NOT_FOUND;
}

onewire_err_t onewire_bus_write_bit(onewire_bus_handle_t bus, uint8_t tx_bit)
{
	uint32_t key;

	if (bus == NULL)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	key = onewire_enter_critical();
	onewire_pin_mode_output_pp(bus);
	ONEWIRE_GPIO_SET_LOW(bus->dq_port, bus->dq_pin);

	if ((tx_bit & 0x01U) != 0U)
	{
		onewire_delay_us(ONEWIRE_SLOT_START_US);
		onewire_bus_release(bus);
		onewire_delay_us(ONEWIRE_SLOT_BIT_US + ONEWIRE_SLOT_RECOVERY_US);
	}
	else
	{
		onewire_delay_us(ONEWIRE_SLOT_START_US + ONEWIRE_SLOT_BIT_US);
		onewire_bus_release(bus);
		onewire_delay_us(ONEWIRE_SLOT_RECOVERY_US);
	}

	onewire_exit_critical(key);
	return ONEWIRE_OK;
}

onewire_err_t onewire_bus_read_bit(onewire_bus_handle_t bus, uint8_t *rx_bit)
{
	uint32_t key;
	uint8_t  bit_value;

	if (bus == NULL || rx_bit == NULL)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	key = onewire_enter_critical();

	onewire_pin_mode_output_pp(bus);
	ONEWIRE_GPIO_SET_LOW(bus->dq_port, bus->dq_pin);
	onewire_delay_us(ONEWIRE_SLOT_START_US);
	onewire_bus_release(bus);
	onewire_delay_us(ONEWIRE_SLOT_SAMPLE_US);
	bit_value = (uint8_t)ONEWIRE_GPIO_READ(bus->dq_port, bus->dq_pin);
	onewire_delay_us((ONEWIRE_SLOT_BIT_US + ONEWIRE_SLOT_RECOVERY_US) - ONEWIRE_SLOT_SAMPLE_US);

	onewire_exit_critical(key);
	*rx_bit = (uint8_t)(bit_value & 0x01U);
	return ONEWIRE_OK;
}

onewire_err_t onewire_bus_write_bytes(onewire_bus_handle_t bus, const uint8_t *tx_data, uint8_t tx_data_size)
{
	uint8_t       i;
	uint8_t       bit;
	onewire_err_t ret;
	uint8_t       byte_value;

	if (bus == NULL || tx_data == NULL || tx_data_size == 0U)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	for (i = 0U; i < tx_data_size; i++)
	{
		byte_value = tx_data[i];
		for (bit = 0U; bit < 8U; bit++)
		{
			ret = onewire_bus_write_bit(bus, (uint8_t)(byte_value & 0x01U));
			if (ret != ONEWIRE_OK)
			{
				return ret;
			}
			byte_value >>= 1;
		}
	}
	return ONEWIRE_OK;
}

onewire_err_t onewire_bus_read_bytes(onewire_bus_handle_t bus, uint8_t *rx_buf, uint16_t rx_buf_size)
{
	uint16_t      i;
	uint8_t       bit;
	uint8_t       bit_idx;
	uint8_t       value;
	onewire_err_t ret;

	if (bus == NULL || rx_buf == NULL || rx_buf_size == 0U)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	for (i = 0U; i < rx_buf_size; i++)
	{
		value = 0U;
		for (bit_idx = 0U; bit_idx < 8U; bit_idx++)
		{
			ret = onewire_bus_read_bit(bus, &bit);
			if (ret != ONEWIRE_OK)
			{
				return ret;
			}
			value |= (uint8_t)((bit & 0x01U) << bit_idx);
		}
		rx_buf[i] = value;
	}
	return ONEWIRE_OK;
}

/* ===== CRC8 ===== */
static const uint8_t g_dallas_crc8_table[256] = {
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
    157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
    35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
    190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
    70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
    219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
    101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
    248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
    140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
    17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
    175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
    50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
    202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
    87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
    233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
    116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

uint8_t onewire_crc8(uint8_t init_crc, const uint8_t *input, uint16_t input_size)
{
	uint16_t i;
	uint8_t  crc = init_crc;

	if (input == NULL)
	{
		return crc;
	}

	for (i = 0U; i < input_size; i++)
	{
		crc = g_dallas_crc8_table[crc ^ input[i]];
	}
	return crc;
}

/* ===== device iterator (ROM Search) ===== */
onewire_err_t onewire_new_device_iter(onewire_bus_handle_t bus, onewire_device_iter_handle_t *ret_iter)
{
	struct onewire_device_iter_t *iter;

	if (bus == NULL || ret_iter == NULL)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	iter = (struct onewire_device_iter_t *)malloc(sizeof(struct onewire_device_iter_t));
	if (iter == NULL)
	{
		return ONEWIRE_ERR_NO_MEM;
	}

	iter->bus = bus;
	iter->last_discrepancy = 0U;
	iter->is_last_device = 0U;
	memset(iter->rom_number, 0, sizeof(iter->rom_number));
	*ret_iter = iter;
	return ONEWIRE_OK;
}

onewire_err_t onewire_del_device_iter(onewire_device_iter_handle_t iter)
{
	if (iter == NULL)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}

	free(iter);
	return ONEWIRE_OK;
}

onewire_err_t onewire_device_iter_get_next(onewire_device_iter_handle_t iter, onewire_device_t *dev)
{
	uint16_t      rom_bit_index;
	uint8_t       last_zero = 0U;
	uint8_t       rom_byte_index;
	uint8_t       rom_bit_mask;
	uint8_t       rom_bit;
	uint8_t       rom_bit_complement;
	uint8_t       search_direction;
	uint8_t       search_cmd;
	onewire_err_t ret;

	if (iter == NULL || dev == NULL || iter->bus == NULL)
	{
		return ONEWIRE_ERR_INVALID_ARG;
	}
	if (iter->is_last_device != 0U)
	{
		return ONEWIRE_ERR_NOT_FOUND;
	}

	ret = onewire_bus_reset(iter->bus);
	if (ret != ONEWIRE_OK)
	{
		return ret;
	}

	search_cmd = ONEWIRE_CMD_SEARCH_NORMAL;
	ret = onewire_bus_write_bytes(iter->bus, &search_cmd, 1U);
	if (ret != ONEWIRE_OK)
	{
		return ret;
	}

	for (rom_bit_index = 0U; rom_bit_index < 64U; rom_bit_index++)
	{
		rom_byte_index = (uint8_t)(rom_bit_index / 8U);
		rom_bit_mask = (uint8_t)(1U << (rom_bit_index % 8U));

		ret = onewire_bus_read_bit(iter->bus, &rom_bit);
		if (ret != ONEWIRE_OK)
		{
			return ret;
		}
		ret = onewire_bus_read_bit(iter->bus, &rom_bit_complement);
		if (ret != ONEWIRE_OK)
		{
			return ret;
		}

		if ((rom_bit != 0U) && (rom_bit_complement != 0U))
		{
			return ONEWIRE_ERR_NOT_FOUND;
		}

		if (rom_bit != rom_bit_complement)
		{
			search_direction = rom_bit;
		}
		else
		{
			if (rom_bit_index < iter->last_discrepancy)
			{
				search_direction = (iter->rom_number[rom_byte_index] & rom_bit_mask) ? 1U : 0U;
			}
			else
			{
				search_direction = (rom_bit_index == iter->last_discrepancy) ? 1U : 0U;
			}

			if (search_direction == 0U)
			{
				last_zero = (uint8_t)rom_bit_index;
			}
		}

		if (search_direction != 0U)
		{
			iter->rom_number[rom_byte_index] |= rom_bit_mask;
		}
		else
		{
			iter->rom_number[rom_byte_index] &= (uint8_t)(~rom_bit_mask);
		}

		ret = onewire_bus_write_bit(iter->bus, search_direction);
		if (ret != ONEWIRE_OK)
		{
			return ret;
		}
	}

	iter->last_discrepancy = last_zero;
	if (iter->last_discrepancy == 0U)
	{
		iter->is_last_device = 1U;
	}

	if (onewire_crc8(0U, iter->rom_number, 7U) != iter->rom_number[7])
	{
		return ONEWIRE_ERR_INVALID_CRC;
	}

	dev->bus = iter->bus;
	dev->address = ((uint64_t)iter->rom_number[0]) |
	               ((uint64_t)iter->rom_number[1] << 8) |
	               ((uint64_t)iter->rom_number[2] << 16) |
	               ((uint64_t)iter->rom_number[3] << 24) |
	               ((uint64_t)iter->rom_number[4] << 32) |
	               ((uint64_t)iter->rom_number[5] << 40) |
	               ((uint64_t)iter->rom_number[6] << 48) |
	               ((uint64_t)iter->rom_number[7] << 56);

	return ONEWIRE_OK;
}
