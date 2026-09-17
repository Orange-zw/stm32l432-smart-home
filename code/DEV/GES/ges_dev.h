#ifndef __GES_DEV_H__
#define __GES_DEV_H__

#include <stdint.h>
#include "stm32f10x.h"

/* PAJ7620U2 工作状态 */
typedef enum
{
	GES_STATE_UNINIT = 0,
	GES_STATE_READY = 1,
	GES_STATE_ERROR = 2
} GES_State_t;

typedef struct
{
	GPIO_TypeDef *scl_port;
	uint16_t      scl_pin;
	GPIO_TypeDef *sda_port;
	uint16_t      sda_pin;
	GES_State_t   state;
	uint8_t       is_initialized;
} GES_dev_t;
typedef GES_dev_t *GES_handle_t;

/* 手势位定义（低8位来自0x43，高位bit8来自0x44 bit0） */
typedef enum
{
	GES_INVALID = 0,
	GES_UP = (1U << 0),
	GES_DOWN = (1U << 1),
	GES_LEFT = (1U << 2),
	GES_RIGHT = (1U << 3),
	GES_FORWARD = (1U << 4),
	GES_BACKWARD = (1U << 5),
	GES_CLOCKWISE = (1U << 6),
	GES_ANTICLOCKWISE = (1U << 7),
	GES_WAVE = (1U << 8)
} GES_DataFlag_t;

/* 生命周期接口 */
GES_handle_t GES_Create(GPIO_TypeDef *scl_port, uint16_t scl_pin, GPIO_TypeDef *sda_port, uint16_t sda_pin);
void         GES_Destroy(GES_handle_t dev);

/* 数据接口 */
GES_State_t    GES_get_state(GES_handle_t dev);
GES_DataFlag_t GES_get_data(GES_handle_t dev);

#endif /* __GES_DEV_H__ */
