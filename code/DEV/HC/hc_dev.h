#ifndef __HC_DEV_H__
#define __HC_DEV_H__

#include "stm32f10x.h"
#include "../../Driver/TIM_IC/tim_ic_driver.h"

#define HC_MAX_DEVICES            (8)
#define HC_DEFAULT_TIMEOUT_US     (30000U)
#define HC_DEFAULT_TRIGGER_US     (20U)
#define HC_DISTANCE_CM_FROM_US(x) ((float)(x) / 58.0f)

typedef struct
{
	GPIO_TypeDef    *trig_port;
	u16              trig_pin;
	TIM_IC_handle_t  echo_ic;
	u32              trigger_pulse_us;
	u8               is_initialized;
} HC_dev_t;
typedef HC_dev_t *HC_handle_t;

HC_handle_t HC_Create(GPIO_TypeDef *trig_port, u16 trig_pin, GPIO_TypeDef *echo_port, u16 echo_pin);
void        HC_Destroy(HC_handle_t dev);
float       HC_ReadDis(HC_handle_t dev, u32 timeout_us);

#endif /* __HC_DEV_H__ */
