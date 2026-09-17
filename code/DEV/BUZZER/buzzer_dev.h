#ifndef __BUZZER_DEV_H__
#define __BUZZER_DEV_H__

#include "DO.h"
#include "pwm_driver.h"

typedef enum
{
	BUZZER_DRV_GPIO = 0,
	BUZZER_DRV_PWM = 1
} buzzer_driver_type_t;

typedef struct
{
	buzzer_driver_type_t type;
	GPIO_TypeDef        *gpio_port;
	uint16_t             gpio_pin;
	DO_handle_t          do_dev;
	PWM_Handle_t        *pwm_dev;
	uint32_t             pwm_freq_hz;
} buzzer_dev_t;

typedef buzzer_dev_t *buzzer_handle_t;

/* 创建蜂鸣器设备：创建时指定驱动类型与连接GPIO引脚 */
buzzer_handle_t Buzzer_Create(buzzer_driver_type_t type, GPIO_TypeDef *gpio_port, uint16_t gpio_pin);

/* 销毁蜂鸣器设备 */
void Buzzer_Destroy(buzzer_handle_t buzzer);

/* 通用开关API：GPIO和PWM类型均可调用 */
void Buzzer_Open(buzzer_handle_t buzzer);
void Buzzer_Close(buzzer_handle_t buzzer);
void Buzzer_SetFreq(buzzer_handle_t buzzer, uint32_t freq_hz);

/* GPIO驱动API：仅对 BUZZER_DRV_GPIO 生效 */
void Buzzer_BeepMs(buzzer_handle_t buzzer, uint32_t duration_ms);

/* PWM驱动API：仅对 BUZZER_DRV_PWM 生效 */
void Buzzer_BeepPwm(buzzer_handle_t buzzer, uint32_t duration_ms, uint32_t freq_hz);

#endif /* __BUZZER_DEV_H__ */
