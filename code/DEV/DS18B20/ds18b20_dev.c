#include "ds18b20_dev.h"
#include "delay.h"

extern void *malloc(unsigned long long size);
extern void  free(void *ptr);

#define DS18B20_CMD_CONVERT_T (0x44U)
#define DS18B20_CMD_READ_SCRATCHPAD (0xBEU)
#define DS18B20_CONVERT_TIME_MS (750U)

static DS18B20_CreateError_t g_ds18b20_last_create_error = DS18B20_CREATE_OK;

static uint8_t DS18B20_BusReset(onewire_bus_handle_t bus);
static uint8_t DS18B20_BusWriteByte(onewire_bus_handle_t bus, uint8_t data);
static uint8_t DS18B20_BusReadByte(onewire_bus_handle_t bus, uint8_t *data);
static uint8_t DS18B20_StartConvert(DS18B20_dev_t *dev);
static uint8_t DS18B20_ReadRawTemp(DS18B20_dev_t *dev, int16_t *raw_temp);

static uint8_t DS18B20_BusReset(onewire_bus_handle_t bus)
{
	return (onewire_bus_reset(bus) == ONEWIRE_OK) ? 1U : 0U;
}

static uint8_t DS18B20_BusWriteByte(onewire_bus_handle_t bus, uint8_t data)
{
	return (onewire_bus_write_bytes(bus, &data, 1U) == ONEWIRE_OK) ? 1U : 0U;
}

static uint8_t DS18B20_BusReadByte(onewire_bus_handle_t bus, uint8_t *data)
{
	return (onewire_bus_read_bytes(bus, data, 1U) == ONEWIRE_OK) ? 1U : 0U;
}

static uint8_t DS18B20_StartConvert(DS18B20_dev_t *dev)
{
	if (DS18B20_BusReset(dev->ow_bus) == 0U)
	{
		return 0U;
	}
	if (DS18B20_BusWriteByte(dev->ow_bus, ONEWIRE_CMD_SKIP_ROM) == 0U)
	{
		return 0U;
	}
	if (DS18B20_BusWriteByte(dev->ow_bus, DS18B20_CMD_CONVERT_T) == 0U)
	{
		return 0U;
	}
	return 1U;
}

static uint8_t DS18B20_ReadRawTemp(DS18B20_dev_t *dev, int16_t *raw_temp)
{
	uint8_t scratchpad[9];
	uint8_t i;
	uint8_t all_zero = 1U;

	if (raw_temp == 0)
	{
		return 0U;
	}
	if (DS18B20_BusReset(dev->ow_bus) == 0U)
	{
		return 0U;
	}
	if (DS18B20_BusWriteByte(dev->ow_bus, ONEWIRE_CMD_SKIP_ROM) == 0U)
	{
		return 0U;
	}
	if (DS18B20_BusWriteByte(dev->ow_bus, DS18B20_CMD_READ_SCRATCHPAD) == 0U)
	{
		return 0U;
	}

	for (i = 0U; i < 9U; i++)
	{
		if (DS18B20_BusReadByte(dev->ow_bus, &scratchpad[i]) == 0U)
		{
			return 0U;
		}
	}

	for (i = 0U; i < 8U; i++)
	{
		if (scratchpad[i] != 0U)
		{
			all_zero = 0U;
			break;
		}
	}
	if (all_zero != 0U)
	{
		return 0U;
	}

	if (onewire_crc8(0U, scratchpad, 8U) != scratchpad[8])
	{
		return 0U;
	}

	*raw_temp = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
	return 1U;
}

DS18B20_handle_t DS18B20_Create(GPIO_TypeDef *dq_port, uint16_t dq_pin)
{
	DS18B20_dev_t         *dev = 0;
	onewire_bus_config_t   bus_cfg;
	uint8_t                detected = 0U;
	uint8_t                retry;
	g_ds18b20_last_create_error = DS18B20_CREATE_OK;

	if (dq_port == 0 || dq_pin == 0U)
	{
		g_ds18b20_last_create_error = DS18B20_CREATE_ERR_INVALID_ARG;
		return 0;
	}

	dev = (DS18B20_dev_t *)malloc(sizeof(DS18B20_dev_t));
	if (dev == 0)
	{
		g_ds18b20_last_create_error = DS18B20_CREATE_ERR_NO_MEM;
		return 0;
	}

	dev->dq_port = dq_port;
	dev->dq_pin = dq_pin;
	dev->ow_bus = 0;
	dev->state = DS18B20_STATE_UNINIT;
	dev->is_initialized = 0U;

	bus_cfg.dq_port = dq_port;
	bus_cfg.dq_pin = dq_pin;
	bus_cfg.flags.en_pull_up = 1U;
	if (onewire_new_bus_gpio(&bus_cfg, &dev->ow_bus) != ONEWIRE_OK)
	{
		g_ds18b20_last_create_error = DS18B20_CREATE_ERR_BUS_NEW;
		free(dev);
		return 0;
	}

	/*
	 * 设备上电早期可能存在短暂不应答，创建阶段重试几次以降低误判。
	 * 与旧驱动行为对齐，优先保证“能识别到设备”。
	 */
	delay_ms(5);
	for (retry = 0U; retry < 5U; retry++)
	{
		if (DS18B20_BusReset(dev->ow_bus) != 0U)
		{
			detected = 1U;
			break;
		}
		delay_ms(2);
	}
	if (detected == 0U)
	{
		g_ds18b20_last_create_error = DS18B20_CREATE_ERR_NO_DEVICE;
		(void)onewire_bus_del(dev->ow_bus);
		dev->ow_bus = 0;
		dev->state = DS18B20_STATE_ERROR;
		free(dev);
		return 0;
	}

	dev->is_initialized = 1U;
	dev->state = DS18B20_STATE_READY;
	return dev;
}

DS18B20_CreateError_t DS18B20_GetLastCreateError(void)
{
	return g_ds18b20_last_create_error;
}

void DS18B20_Destroy(DS18B20_handle_t dev)
{
	if (dev == 0)
	{
		return;
	}

	if (dev->ow_bus != 0)
	{
		(void)onewire_bus_del(dev->ow_bus);
		dev->ow_bus = 0;
	}
	dev->state = DS18B20_STATE_UNINIT;
	dev->is_initialized = 0U;
	free(dev);
}

DS18B20_State_t DS18B20_get_state(DS18B20_handle_t dev)
{
	if (dev == 0)
	{
		return DS18B20_STATE_ERROR;
	}
	return dev->state;
}

uint8_t DS18B20_ReadTempC(DS18B20_handle_t dev, float *temp_c)
{
	int16_t raw_temp = 0;

	if ((dev == 0) || (temp_c == 0) || (dev->is_initialized == 0U) || (dev->ow_bus == 0))
	{
		return 0U;
	}

	if (DS18B20_StartConvert(dev) == 0U)
	{
		dev->state = DS18B20_STATE_ERROR;
		return 0U;
	}

	delay_ms(DS18B20_CONVERT_TIME_MS);
	if (DS18B20_ReadRawTemp(dev, &raw_temp) == 0U)
	{
		dev->state = DS18B20_STATE_ERROR;
		return 0U;
	}

	*temp_c = (float)raw_temp / 16.0f;
	dev->state = DS18B20_STATE_READY;
	return 1U;
}
