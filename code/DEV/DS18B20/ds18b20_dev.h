#ifndef __DS18B20_DEV_H__
#define __DS18B20_DEV_H__

#include <stdint.h>
#include "stm32f10x.h"
#include "../../Driver/ONE_WIRE/onewire_bus.h"

#define DS18B20_INVALID_TEMP_C10 (32767)

typedef enum
{
	DS18B20_STATE_UNINIT = 0,
	DS18B20_STATE_READY = 1,
	DS18B20_STATE_ERROR = 2
} DS18B20_State_t;

typedef enum
{
	DS18B20_CREATE_OK = 0,
	DS18B20_CREATE_ERR_INVALID_ARG = 1,
	DS18B20_CREATE_ERR_NO_MEM = 2,
	DS18B20_CREATE_ERR_BUS_NEW = 3,
	DS18B20_CREATE_ERR_NO_DEVICE = 4
} DS18B20_CreateError_t;

typedef struct
{
	GPIO_TypeDef    *dq_port;
	uint16_t         dq_pin;
	onewire_bus_handle_t ow_bus;
	DS18B20_State_t  state;
	uint8_t          is_initialized;
} DS18B20_dev_t;
typedef DS18B20_dev_t *DS18B20_handle_t;

DS18B20_handle_t DS18B20_Create(GPIO_TypeDef *dq_port, uint16_t dq_pin);
void             DS18B20_Destroy(DS18B20_handle_t dev);
DS18B20_CreateError_t DS18B20_GetLastCreateError(void);

DS18B20_State_t DS18B20_get_state(DS18B20_handle_t dev);
uint8_t         DS18B20_ReadTempC(DS18B20_handle_t dev, float *temp_c);

#endif /* __DS18B20_DEV_H__ */
