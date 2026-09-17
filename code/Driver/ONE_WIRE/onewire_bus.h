#ifndef __ONEWIRE_BUS_H__
#define __ONEWIRE_BUS_H__

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	ONEWIRE_OK = 0,
	ONEWIRE_ERR_INVALID_ARG = -1,
	ONEWIRE_ERR_NO_MEM = -2,
	ONEWIRE_ERR_NOT_FOUND = -3,
	ONEWIRE_ERR_INVALID_CRC = -4,
	ONEWIRE_ERR_FAIL = -5
} onewire_err_t;

#define ONEWIRE_CMD_SEARCH_NORMAL      (0xF0U)
#define ONEWIRE_CMD_MATCH_ROM          (0x55U)
#define ONEWIRE_CMD_SKIP_ROM           (0xCCU)
#define ONEWIRE_CMD_SEARCH_ALARM       (0xECU)
#define ONEWIRE_CMD_READ_POWER_SUPPLY  (0xB4U)

typedef struct onewire_bus_t *onewire_bus_handle_t;
typedef unsigned long long onewire_device_address_t;
typedef struct onewire_device_iter_t *onewire_device_iter_handle_t;

typedef struct
{
	GPIO_TypeDef *dq_port;
	uint16_t      dq_pin;
	struct
	{
		uint8_t en_pull_up : 1;
	} flags;
} onewire_bus_config_t;

typedef struct
{
	onewire_bus_handle_t      bus;
	onewire_device_address_t  address;
} onewire_device_t;

onewire_err_t onewire_new_bus_gpio(const onewire_bus_config_t *bus_config, onewire_bus_handle_t *ret_bus);
onewire_err_t onewire_bus_del(onewire_bus_handle_t bus);
onewire_err_t onewire_bus_reset(onewire_bus_handle_t bus);
onewire_err_t onewire_bus_write_bit(onewire_bus_handle_t bus, uint8_t tx_bit);
onewire_err_t onewire_bus_read_bit(onewire_bus_handle_t bus, uint8_t *rx_bit);
onewire_err_t onewire_bus_write_bytes(onewire_bus_handle_t bus, const uint8_t *tx_data, uint8_t tx_data_size);
onewire_err_t onewire_bus_read_bytes(onewire_bus_handle_t bus, uint8_t *rx_buf, uint16_t rx_buf_size);

onewire_err_t onewire_new_device_iter(onewire_bus_handle_t bus, onewire_device_iter_handle_t *ret_iter);
onewire_err_t onewire_del_device_iter(onewire_device_iter_handle_t iter);
onewire_err_t onewire_device_iter_get_next(onewire_device_iter_handle_t iter, onewire_device_t *dev);

uint8_t onewire_crc8(uint8_t init_crc, const uint8_t *input, uint16_t input_size);

#ifdef __cplusplus
}
#endif

#endif /* __ONEWIRE_BUS_H__ */
