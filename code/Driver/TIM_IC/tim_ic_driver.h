#ifndef __TIM_IC_DRIVER_H__
#define __TIM_IC_DRIVER_H__

#include <stdbool.h>
#include "stm32f10x.h"

#define TIM_IC_OP(dev, op, ...) TIM_IC_GetOperations()->op(dev, ##__VA_ARGS__)

typedef enum
{
	TIM_IC_STATE_READY = 0,
	TIM_IC_STATE_BUSY,
	TIM_IC_STATE_ERROR
} TIM_IC_State;

typedef struct
{
	u32  tick_hz;
	u16  period;
	u16  ic_filter;
	bool enable_update_irq;
} TIM_IC_Config;

typedef void (*TIM_IC_PulseCallback)(u32 pulse_width_us);
typedef void (*TIM_IC_PeriodCallback)(u32 period_us);
typedef void (*TIM_IC_ErrorCallback)(void);

typedef struct
{
	GPIO_TypeDef *gpio_port;
	u16           gpio_pin;
	TIM_TypeDef  *timer;
	u16           channel;
	bool          is_remapped;
	u32           remap_value;
} TIM_IC_PinMap;

typedef struct
{
	TIM_TypeDef  *timer;
	u16           channel;
	GPIO_TypeDef *gpio_port;
	u16           gpio_pin;
	bool          is_initialized;
	bool          enabled;
	bool          waiting_falling;
	TIM_IC_State  state;
	TIM_IC_Config config;

	volatile u32  overflow_count;
	volatile u32  last_rise_capture;
	volatile u32  pulse_width_ticks;
	volatile u32  period_ticks;
	volatile bool pulse_ready;
	volatile bool period_ready;
	volatile bool has_last_rise;

	TIM_IC_PulseCallback  pulse_callback;
	TIM_IC_PeriodCallback period_callback;
	TIM_IC_ErrorCallback  error_callback;
} TIM_IC_dev_t;
typedef TIM_IC_dev_t *TIM_IC_handle_t;

typedef struct
{
	bool (*init)(TIM_IC_handle_t htim_ic);
	bool (*deinit)(TIM_IC_handle_t htim_ic);
	TIM_IC_handle_t (*create)(GPIO_TypeDef *gpio_port, u16 gpio_pin);
	void (*destroy)(TIM_IC_handle_t htim_ic);
	bool (*start)(TIM_IC_handle_t htim_ic);
	bool (*stop)(TIM_IC_handle_t htim_ic);
	bool (*set_config)(TIM_IC_handle_t htim_ic, const TIM_IC_Config *config);
	bool (*set_pulse_callback)(TIM_IC_handle_t htim_ic, TIM_IC_PulseCallback callback);
	bool (*set_period_callback)(TIM_IC_handle_t htim_ic, TIM_IC_PeriodCallback callback);
	bool (*set_error_callback)(TIM_IC_handle_t htim_ic, TIM_IC_ErrorCallback callback);
	bool (*read_pulse_us)(TIM_IC_handle_t htim_ic, u32 *pulse_width_us);
	bool (*read_period_us)(TIM_IC_handle_t htim_ic, u32 *period_us);
	bool (*is_ready)(TIM_IC_handle_t htim_ic);
	void (*irq_handler)(TIM_TypeDef *timer);
} TIM_IC_Operations;

TIM_IC_handle_t    TIM_IC_Create(GPIO_TypeDef *gpio_port, u16 gpio_pin);
void               TIM_IC_Destroy(TIM_IC_handle_t htim_ic);
TIM_IC_Operations *TIM_IC_GetOperations(void);

/* Generic dispatcher (can be called by user ISR). */
void TIM_IC_IRQHandler(TIM_TypeDef *timer);

#endif /* __TIM_IC_DRIVER_H__ */
