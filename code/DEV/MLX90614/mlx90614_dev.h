#ifndef __MLX90614_DEV_H__
#define __MLX90614_DEV_H__

#include <stdint.h>
#include "stm32f10x.h"

typedef enum
{
	MLX_STATE_UNINIT = 0,
	MLX_STATE_READY = 1,
	MLX_STATE_ERROR = 2
} MLX_State_t;

typedef struct
{
	GPIO_TypeDef *scl_port;
	uint16_t      scl_pin;
	GPIO_TypeDef *sda_port;
	uint16_t      sda_pin;
	MLX_State_t   state;
	uint8_t       is_initialized;
} MLX_dev_t;
typedef MLX_dev_t *MLX_handle_t;

MLX_handle_t MLX_Init(GPIO_TypeDef *scl_port, uint16_t scl_pin, GPIO_TypeDef *sda_port, uint16_t sda_pin);
void         MLX_DeInit(MLX_handle_t dev);
uint8_t      MLX_ReadTemp(MLX_handle_t dev, float *temp);
MLX_State_t  MLX_GetState(MLX_handle_t dev);

#endif /* __MLX90614_DEV_H__ */
