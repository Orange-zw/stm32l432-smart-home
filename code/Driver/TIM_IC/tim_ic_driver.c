#include "tim_ic_driver.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __weak
#define __weak __attribute__((weak))
#endif

#define TIM_IC_MAX_INSTANCES (8)

static bool TIM_IC_DriverInit(TIM_IC_handle_t htim_ic);
static bool TIM_IC_DriverDeinit(TIM_IC_handle_t htim_ic);
static bool TIM_IC_Start(TIM_IC_handle_t htim_ic);
static bool TIM_IC_Stop(TIM_IC_handle_t htim_ic);
static bool TIM_IC_SetConfig(TIM_IC_handle_t htim_ic, const TIM_IC_Config *config);
static bool TIM_IC_SetPulseCallback(TIM_IC_handle_t htim_ic, TIM_IC_PulseCallback callback);
static bool TIM_IC_SetPeriodCallback(TIM_IC_handle_t htim_ic, TIM_IC_PeriodCallback callback);
static bool TIM_IC_SetErrorCallback(TIM_IC_handle_t htim_ic, TIM_IC_ErrorCallback callback);
static bool TIM_IC_ReadPulseUs(TIM_IC_handle_t htim_ic, u32 *pulse_width_us);
static bool TIM_IC_ReadPeriodUs(TIM_IC_handle_t htim_ic, u32 *period_us);
static bool TIM_IC_IsReady(TIM_IC_handle_t htim_ic);

static void                 TIM_IC_EnableGPIOClock(GPIO_TypeDef *gpio_port);
static void                 TIM_IC_EnableTimerClock(TIM_TypeDef *timer);
static void                 TIM_IC_InitNVIC(TIM_TypeDef *timer);
static void                 TIM_IC_ConfigureGPIO(const TIM_IC_PinMap *pin_map);
static void                 TIM_IC_ConfigureTimer(TIM_IC_handle_t htim_ic);
static void                 TIM_IC_ConfigureChannel(TIM_IC_handle_t htim_ic, u16 polarity);
static const TIM_IC_PinMap *TIM_IC_FindPinMap(GPIO_TypeDef *gpio_port, u16 gpio_pin);
static TIM_IC_handle_t      TIM_IC_GetInstanceByTimerChannel(TIM_TypeDef *timer, u16 channel);
static u8                   TIM_IC_ActiveUserCount(TIM_TypeDef *timer);
static u32                  TIM_IC_GetCapture(TIM_TypeDef *timer, u16 channel);
static u32                  TIM_IC_GetCCInterrupt(TIM_TypeDef *timer, u16 channel);
static u32                  TIM_IC_TicksToUs(TIM_IC_handle_t htim_ic, u32 ticks);
static u32                  TIM_IC_CalcTickHz(TIM_TypeDef *timer);

static TIM_IC_Operations tim_ic_ops = {
    .init = TIM_IC_DriverInit,
    .deinit = TIM_IC_DriverDeinit,
    .create = TIM_IC_Create,
    .destroy = TIM_IC_Destroy,
    .start = TIM_IC_Start,
    .stop = TIM_IC_Stop,
    .set_config = TIM_IC_SetConfig,
    .set_pulse_callback = TIM_IC_SetPulseCallback,
    .set_period_callback = TIM_IC_SetPeriodCallback,
    .set_error_callback = TIM_IC_SetErrorCallback,
    .read_pulse_us = TIM_IC_ReadPulseUs,
    .read_period_us = TIM_IC_ReadPeriodUs,
    .is_ready = TIM_IC_IsReady,
    .irq_handler = TIM_IC_IRQHandler};

/* Input capture pin map (default channels + common remaps). */
static const TIM_IC_PinMap tim_ic_pin_map[] = {
    {GPIOA, GPIO_Pin_8, TIM1, TIM_Channel_1, false, 0},
    {GPIOA, GPIO_Pin_9, TIM1, TIM_Channel_2, false, 0},
    {GPIOA, GPIO_Pin_10, TIM1, TIM_Channel_3, false, 0},
    {GPIOA, GPIO_Pin_11, TIM1, TIM_Channel_4, false, 0},

    {GPIOA, GPIO_Pin_0, TIM2, TIM_Channel_1, false, 0},
    {GPIOA, GPIO_Pin_1, TIM2, TIM_Channel_2, false, 0},
    {GPIOA, GPIO_Pin_2, TIM2, TIM_Channel_3, false, 0},
    {GPIOA, GPIO_Pin_3, TIM2, TIM_Channel_4, false, 0},
    {GPIOA, GPIO_Pin_15, TIM2, TIM_Channel_1, true, GPIO_PartialRemap1_TIM2},
    {GPIOB, GPIO_Pin_3, TIM2, TIM_Channel_2, true, GPIO_PartialRemap1_TIM2},
    {GPIOB, GPIO_Pin_10, TIM2, TIM_Channel_3, true, GPIO_PartialRemap2_TIM2},
    {GPIOB, GPIO_Pin_11, TIM2, TIM_Channel_4, true, GPIO_PartialRemap2_TIM2},

    {GPIOA, GPIO_Pin_6, TIM3, TIM_Channel_1, false, 0},
    {GPIOA, GPIO_Pin_7, TIM3, TIM_Channel_2, false, 0},
    {GPIOB, GPIO_Pin_0, TIM3, TIM_Channel_3, false, 0},
    {GPIOB, GPIO_Pin_1, TIM3, TIM_Channel_4, false, 0},
    {GPIOB, GPIO_Pin_4, TIM3, TIM_Channel_1, true, GPIO_PartialRemap_TIM3},
    {GPIOB, GPIO_Pin_5, TIM3, TIM_Channel_2, true, GPIO_PartialRemap_TIM3},

    {GPIOB, GPIO_Pin_6, TIM4, TIM_Channel_1, false, 0},
    {GPIOB, GPIO_Pin_7, TIM4, TIM_Channel_2, false, 0},
    {GPIOB, GPIO_Pin_8, TIM4, TIM_Channel_3, false, 0},
    {GPIOB, GPIO_Pin_9, TIM4, TIM_Channel_4, false, 0},
#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
    {GPIOD, GPIO_Pin_12, TIM4, TIM_Channel_1, true, GPIO_Remap_TIM4},
    {GPIOD, GPIO_Pin_13, TIM4, TIM_Channel_2, true, GPIO_Remap_TIM4},
    {GPIOD, GPIO_Pin_14, TIM4, TIM_Channel_3, true, GPIO_Remap_TIM4},
    {GPIOD, GPIO_Pin_15, TIM4, TIM_Channel_4, true, GPIO_Remap_TIM4},
#endif
    {NULL, 0, NULL, 0, false, 0}};

static TIM_IC_dev_t    instance_pool[TIM_IC_MAX_INSTANCES];
static TIM_IC_handle_t instances[TIM_IC_MAX_INSTANCES] = {NULL};

static bool TIM_IC_DriverInit(TIM_IC_handle_t htim_ic)
{
	if (htim_ic == NULL)
	{
		return false;
	}
	if (htim_ic->is_initialized)
	{
		return true;
	}

	TIM_IC_ConfigureTimer(htim_ic);
	TIM_IC_ConfigureChannel(htim_ic, TIM_ICPolarity_Rising);
	TIM_IC_InitNVIC(htim_ic->timer);

	htim_ic->overflow_count = 0;
	htim_ic->last_rise_capture = 0;
	htim_ic->pulse_width_ticks = 0;
	htim_ic->period_ticks = 0;
	htim_ic->pulse_ready = false;
	htim_ic->period_ready = false;
	htim_ic->has_last_rise = false;
	htim_ic->waiting_falling = false;
	htim_ic->enabled = false;
	htim_ic->state = TIM_IC_STATE_READY;
	htim_ic->is_initialized = true;

	return true;
}

static bool TIM_IC_DriverDeinit(TIM_IC_handle_t htim_ic)
{
	if (htim_ic == NULL)
	{
		return false;
	}
	if (!htim_ic->is_initialized)
	{
		return true;
	}

	TIM_IC_Stop(htim_ic);
	htim_ic->is_initialized = false;
	htim_ic->state = TIM_IC_STATE_READY;
	return true;
}

TIM_IC_handle_t TIM_IC_Create(GPIO_TypeDef *gpio_port, u16 gpio_pin)
{
	const TIM_IC_PinMap *pin_map = TIM_IC_FindPinMap(gpio_port, gpio_pin);
	TIM_IC_handle_t      htim_ic = NULL;

	if (pin_map == NULL)
	{
		return NULL;
	}

	for (int i = 0; i < TIM_IC_MAX_INSTANCES; i++)
	{
		if (instances[i] == NULL)
		{
			htim_ic = &instance_pool[i];
			instances[i] = htim_ic;
			break;
		}
	}

	if (htim_ic == NULL)
	{
		return NULL;
	}

	htim_ic->timer = pin_map->timer;
	htim_ic->channel = pin_map->channel;
	htim_ic->gpio_port = pin_map->gpio_port;
	htim_ic->gpio_pin = pin_map->gpio_pin;
	htim_ic->is_initialized = false;
	htim_ic->enabled = false;
	htim_ic->waiting_falling = false;
	htim_ic->state = TIM_IC_STATE_READY;

	htim_ic->config.tick_hz = 1000000U;
	htim_ic->config.period = 0xFFFFU;
	htim_ic->config.ic_filter = 0;
	htim_ic->config.enable_update_irq = true;

	htim_ic->pulse_callback = NULL;
	htim_ic->period_callback = NULL;
	htim_ic->error_callback = NULL;

	TIM_IC_EnableGPIOClock(pin_map->gpio_port);
	TIM_IC_EnableTimerClock(pin_map->timer);
	TIM_IC_ConfigureGPIO(pin_map);
	if (pin_map->is_remapped)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
		GPIO_PinRemapConfig(pin_map->remap_value, ENABLE);
	}

	if (!TIM_IC_DriverInit(htim_ic))
	{
		TIM_IC_Destroy(htim_ic);
		return NULL;
	}

	return htim_ic;
}

void TIM_IC_Destroy(TIM_IC_handle_t htim_ic)
{
	if (htim_ic == NULL)
	{
		return;
	}

	TIM_IC_DriverDeinit(htim_ic);

	for (int i = 0; i < TIM_IC_MAX_INSTANCES; i++)
	{
		if (instances[i] == htim_ic)
		{
			instances[i] = NULL;
			break;
		}
	}
}

static bool TIM_IC_Start(TIM_IC_handle_t htim_ic)
{
	u32 cc_it = 0;

	if (htim_ic == NULL || !htim_ic->is_initialized)
	{
		return false;
	}

	cc_it = TIM_IC_GetCCInterrupt(htim_ic->timer, htim_ic->channel);
	htim_ic->overflow_count = 0;
	htim_ic->waiting_falling = false;
	htim_ic->state = TIM_IC_STATE_BUSY;

	TIM_SetCounter(htim_ic->timer, 0);
	TIM_IC_ConfigureChannel(htim_ic, TIM_ICPolarity_Rising);
	TIM_ClearITPendingBit(htim_ic->timer, cc_it);
	TIM_ITConfig(htim_ic->timer, cc_it, ENABLE);

	if (htim_ic->config.enable_update_irq)
	{
		TIM_ClearITPendingBit(htim_ic->timer, TIM_IT_Update);
		TIM_ITConfig(htim_ic->timer, TIM_IT_Update, ENABLE);
	}

	TIM_CCxCmd(htim_ic->timer, htim_ic->channel, TIM_CCx_Enable);
	TIM_Cmd(htim_ic->timer, ENABLE);
	htim_ic->enabled = true;
	return true;
}

static bool TIM_IC_Stop(TIM_IC_handle_t htim_ic)
{
	u32 cc_it = 0;

	if (htim_ic == NULL || !htim_ic->is_initialized)
	{
		return false;
	}

	cc_it = TIM_IC_GetCCInterrupt(htim_ic->timer, htim_ic->channel);
	TIM_ITConfig(htim_ic->timer, cc_it, DISABLE);
	TIM_CCxCmd(htim_ic->timer, htim_ic->channel, TIM_CCx_Disable);

	htim_ic->enabled = false;
	htim_ic->state = TIM_IC_STATE_READY;
	htim_ic->waiting_falling = false;

	if (TIM_IC_ActiveUserCount(htim_ic->timer) == 0)
	{
		TIM_ITConfig(htim_ic->timer, TIM_IT_Update, DISABLE);
		TIM_Cmd(htim_ic->timer, DISABLE);
	}
	return true;
}

static bool TIM_IC_SetConfig(TIM_IC_handle_t htim_ic, const TIM_IC_Config *config)
{
	if (htim_ic == NULL || config == NULL)
	{
		return false;
	}
	if (config->tick_hz == 0)
	{
		return false;
	}

	htim_ic->config = *config;
	TIM_IC_ConfigureTimer(htim_ic);
	TIM_IC_ConfigureChannel(htim_ic, htim_ic->waiting_falling ? TIM_ICPolarity_Falling : TIM_ICPolarity_Rising);
	return true;
}

static bool TIM_IC_SetPulseCallback(TIM_IC_handle_t htim_ic, TIM_IC_PulseCallback callback)
{
	if (htim_ic == NULL)
	{
		return false;
	}
	htim_ic->pulse_callback = callback;
	return true;
}

static bool TIM_IC_SetPeriodCallback(TIM_IC_handle_t htim_ic, TIM_IC_PeriodCallback callback)
{
	if (htim_ic == NULL)
	{
		return false;
	}
	htim_ic->period_callback = callback;
	return true;
}

static bool TIM_IC_SetErrorCallback(TIM_IC_handle_t htim_ic, TIM_IC_ErrorCallback callback)
{
	if (htim_ic == NULL)
	{
		return false;
	}
	htim_ic->error_callback = callback;
	return true;
}

static bool TIM_IC_ReadPulseUs(TIM_IC_handle_t htim_ic, u32 *pulse_width_us)
{
	if (htim_ic == NULL || pulse_width_us == NULL)
	{
		return false;
	}
	if (!htim_ic->pulse_ready)
	{
		return false;
	}

	*pulse_width_us = TIM_IC_TicksToUs(htim_ic, htim_ic->pulse_width_ticks);
	htim_ic->pulse_ready = false;
	return true;
}

static bool TIM_IC_ReadPeriodUs(TIM_IC_handle_t htim_ic, u32 *period_us)
{
	if (htim_ic == NULL || period_us == NULL)
	{
		return false;
	}
	if (!htim_ic->period_ready)
	{
		return false;
	}

	*period_us = TIM_IC_TicksToUs(htim_ic, htim_ic->period_ticks);
	htim_ic->period_ready = false;
	return true;
}

static bool TIM_IC_IsReady(TIM_IC_handle_t htim_ic)
{
	return (htim_ic != NULL) && htim_ic->is_initialized;
}

void TIM_IC_IRQHandler(TIM_TypeDef *timer)
{
	u16 channels[] = {TIM_Channel_1, TIM_Channel_2, TIM_Channel_3, TIM_Channel_4};
	u8  count = sizeof(channels) / sizeof(channels[0]);

	/* Update interrupt: increase overflow counter for all active channels on this timer. */
	if (TIM_GetITStatus(timer, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(timer, TIM_IT_Update);
		for (int i = 0; i < TIM_IC_MAX_INSTANCES; i++)
		{
			if (instances[i] && instances[i]->timer == timer && instances[i]->enabled)
			{
				instances[i]->overflow_count++;
			}
		}
	}

	for (u8 i = 0; i < count; i++)
	{
		TIM_IC_handle_t htim_ic = TIM_IC_GetInstanceByTimerChannel(timer, channels[i]);
		u32             cc_it = 0;
		u32             capture = 0;
		u32             total_ticks = 0;
		u32             arr_reload = (u32)timer->ARR + 1U;

		if (htim_ic == NULL || !htim_ic->enabled)
		{
			continue;
		}

		cc_it = TIM_IC_GetCCInterrupt(timer, channels[i]);
		if (TIM_GetITStatus(timer, cc_it) == RESET)
		{
			continue;
		}

		TIM_ClearITPendingBit(timer, cc_it);
		capture = TIM_IC_GetCapture(timer, channels[i]);

		if (!htim_ic->waiting_falling)
		{
			/* Rising edge: optional period from two consecutive rising edges. */
			if (htim_ic->has_last_rise)
			{
				u32 last = htim_ic->last_rise_capture;
				if (capture >= last)
				{
					total_ticks = htim_ic->overflow_count * arr_reload + (capture - last);
				}
				else
				{
					total_ticks = htim_ic->overflow_count * arr_reload + (arr_reload - last + capture);
				}
				htim_ic->period_ticks = total_ticks;
				htim_ic->period_ready = true;
				if (htim_ic->period_callback)
				{
					htim_ic->period_callback(TIM_IC_TicksToUs(htim_ic, htim_ic->period_ticks));
				}
			}
			htim_ic->last_rise_capture = capture;
			htim_ic->has_last_rise = true;
			htim_ic->overflow_count = 0;
			htim_ic->waiting_falling = true;
			TIM_IC_ConfigureChannel(htim_ic, TIM_ICPolarity_Falling);
		}
		else
		{
			/* Falling edge: pulse width calculation from last rising edge. */
			if (capture >= htim_ic->last_rise_capture)
			{
				total_ticks = htim_ic->overflow_count * arr_reload + (capture - htim_ic->last_rise_capture);
			}
			else
			{
				total_ticks = htim_ic->overflow_count * arr_reload +
				              (arr_reload - htim_ic->last_rise_capture + capture);
			}

			htim_ic->pulse_width_ticks = total_ticks;
			htim_ic->pulse_ready = true;
			htim_ic->waiting_falling = false;
			htim_ic->overflow_count = 0;
			htim_ic->state = TIM_IC_STATE_READY;

			if (htim_ic->pulse_callback)
			{
				htim_ic->pulse_callback(TIM_IC_TicksToUs(htim_ic, htim_ic->pulse_width_ticks));
			}
			TIM_IC_ConfigureChannel(htim_ic, TIM_ICPolarity_Rising);
		}
	}
}

static void TIM_IC_EnableGPIOClock(GPIO_TypeDef *gpio_port)
{
	if (gpio_port == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (gpio_port == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
	else if (gpio_port == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if (gpio_port == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}
#endif
}

static void TIM_IC_EnableTimerClock(TIM_TypeDef *timer)
{
	if (timer == TIM1)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	}
	else if (timer == TIM2)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	}
	else if (timer == TIM3)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	}
	else if (timer == TIM4)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	}
}

static void TIM_IC_InitNVIC(TIM_TypeDef *timer)
{
	NVIC_InitTypeDef nvic;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	nvic.NVIC_IRQChannelPreemptionPriority = 1;
	nvic.NVIC_IRQChannelSubPriority = 1;
	nvic.NVIC_IRQChannelCmd = ENABLE;

	if (timer == TIM1)
	{
		nvic.NVIC_IRQChannel = TIM1_CC_IRQn;
	}
	else if (timer == TIM2)
	{
		nvic.NVIC_IRQChannel = TIM2_IRQn;
	}
	else if (timer == TIM3)
	{
		nvic.NVIC_IRQChannel = TIM3_IRQn;
	}
	else
	{
		nvic.NVIC_IRQChannel = TIM4_IRQn;
	}
	NVIC_Init(&nvic);
}

static void TIM_IC_ConfigureGPIO(const TIM_IC_PinMap *pin_map)
{
	GPIO_InitTypeDef gpio;

	gpio.GPIO_Pin = pin_map->gpio_pin;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(pin_map->gpio_port, &gpio);
}

static void TIM_IC_ConfigureTimer(TIM_IC_handle_t htim_ic)
{
	TIM_TimeBaseInitTypeDef tim;
	u32                     timer_clk = TIM_IC_CalcTickHz(htim_ic->timer);
	u32                     psc = 0;

	if (htim_ic->config.tick_hz == 0)
	{
		htim_ic->config.tick_hz = 1000000U;
	}
	psc = timer_clk / htim_ic->config.tick_hz;
	if (psc == 0)
	{
		psc = 1;
	}
	psc -= 1U;
	if (psc > 0xFFFFU)
	{
		psc = 0xFFFFU;
	}

	TIM_TimeBaseStructInit(&tim);
	tim.TIM_Prescaler = (u16)psc;
	tim.TIM_Period = htim_ic->config.period;
	tim.TIM_ClockDivision = TIM_CKD_DIV1;
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	if (htim_ic->timer == TIM1)
	{
		tim.TIM_RepetitionCounter = 0;
	}

	TIM_TimeBaseInit(htim_ic->timer, &tim);
}

static void TIM_IC_ConfigureChannel(TIM_IC_handle_t htim_ic, u16 polarity)
{
	TIM_ICInitTypeDef ic;

	TIM_ICStructInit(&ic);
	ic.TIM_Channel = htim_ic->channel;
	ic.TIM_ICPolarity = polarity;
	ic.TIM_ICSelection = TIM_ICSelection_DirectTI;
	ic.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	ic.TIM_ICFilter = htim_ic->config.ic_filter;
	TIM_ICInit(htim_ic->timer, &ic);
}

static const TIM_IC_PinMap *TIM_IC_FindPinMap(GPIO_TypeDef *gpio_port, u16 gpio_pin)
{
	for (int i = 0; tim_ic_pin_map[i].gpio_port != NULL; i++)
	{
		if (tim_ic_pin_map[i].gpio_port == gpio_port &&
		    tim_ic_pin_map[i].gpio_pin == gpio_pin)
		{
			return &tim_ic_pin_map[i];
		}
	}
	return NULL;
}

static TIM_IC_handle_t TIM_IC_GetInstanceByTimerChannel(TIM_TypeDef *timer, u16 channel)
{
	for (int i = 0; i < TIM_IC_MAX_INSTANCES; i++)
	{
		if (instances[i] &&
		    instances[i]->timer == timer &&
		    instances[i]->channel == channel &&
		    instances[i]->is_initialized)
		{
			return instances[i];
		}
	}
	return NULL;
}

static u8 TIM_IC_ActiveUserCount(TIM_TypeDef *timer)
{
	u8 users = 0;

	for (int i = 0; i < TIM_IC_MAX_INSTANCES; i++)
	{
		if (instances[i] &&
		    instances[i]->timer == timer &&
		    instances[i]->is_initialized &&
		    instances[i]->enabled)
		{
			users++;
		}
	}
	return users;
}

static u32 TIM_IC_GetCapture(TIM_TypeDef *timer, u16 channel)
{
	switch (channel)
	{
	case TIM_Channel_1:
		return TIM_GetCapture1(timer);
	case TIM_Channel_2:
		return TIM_GetCapture2(timer);
	case TIM_Channel_3:
		return TIM_GetCapture3(timer);
	case TIM_Channel_4:
		return TIM_GetCapture4(timer);
	default:
		return 0;
	}
}

static u32 TIM_IC_GetCCInterrupt(TIM_TypeDef *timer, u16 channel)
{
	(void)timer;

	switch (channel)
	{
	case TIM_Channel_1:
		return TIM_IT_CC1;
	case TIM_Channel_2:
		return TIM_IT_CC2;
	case TIM_Channel_3:
		return TIM_IT_CC3;
	case TIM_Channel_4:
		return TIM_IT_CC4;
	default:
		return 0;
	}
}

static u32 TIM_IC_TicksToUs(TIM_IC_handle_t htim_ic, u32 ticks)
{
	if (htim_ic->config.tick_hz == 0)
	{
		return 0;
	}
	return (u32)(((unsigned long long)ticks * 1000000ULL) / htim_ic->config.tick_hz);
}

static u32 TIM_IC_CalcTickHz(TIM_TypeDef *timer)
{
	RCC_ClocksTypeDef clocks;
	u32               pclk = 0;

	RCC_GetClocksFreq(&clocks);
	if (timer == TIM1)
	{
		pclk = clocks.PCLK2_Frequency;
		return (RCC->CFGR & (1U << 13)) ? (pclk * 2U) : pclk;
	}
	pclk = clocks.PCLK1_Frequency;
	return (RCC->CFGR & (1U << 10)) ? (pclk * 2U) : pclk;
}

inline TIM_IC_Operations *TIM_IC_GetOperations(void)
{
	return &tim_ic_ops;
}

/*
 * If a timer ISR conflicts with other modules, comment out that ISR here
 * and implement/merge the dispatch in your own module.
 */
void TIM1_CC_IRQHandler(void)
{
	TIM_IC_IRQHandler(TIM1);
}

// void TIM2_IRQHandler(void)
// {
// 	TIM_IC_IRQHandler(TIM2);
// }

void TIM3_IRQHandler(void)
{
	TIM_IC_IRQHandler(TIM3);
}

void TIM4_IRQHandler(void)
{
	TIM_IC_IRQHandler(TIM4);
}
