/* ============================================================
 * 模块     : PWM 驱动
 * 功能     : PWM 输出与 GPIO→定时器通道映射、重映射与互补通道支持
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

// PWM_dev.c
#include "pwm_driver.h"
#include <stdlib.h>
#include "stdbool.h"

static void     PWM_Start_Impl(PWM_Handle_t *driver);
static void     PWM_Stop_Impl(PWM_Handle_t *driver);
static void     PWM_SetDutyCycle_Impl(PWM_Handle_t *driver, float duty_cycle);
static void     PWM_SetFrequency_Impl(PWM_Handle_t *driver, uint32_t frequency);
static void     PWM_SetConfig_Impl(PWM_Handle_t *driver, uint32_t prescaler, uint32_t period);
static bool     PWM_IsEnabled_Impl(PWM_Handle_t *driver);
static void     PWM_InitHardware(PWM_Handle_t *driver, const PWM_PinMap *pin_map);
static uint16_t PWM_GetDuty_Impl(PWM_Handle_t *driver);
static uint32_t PWM_GetFrequency_Impl(PWM_Handle_t *driver);
static uint32_t PWM_GetPrescaler_Impl(PWM_Handle_t *driver);
static uint32_t PWM_GetPeriod_Impl(PWM_Handle_t *driver);

// 静态操作接口实例
static PWM_Operations pwm_ops = {
    .start = PWM_Start_Impl,
    .stop = PWM_Stop_Impl,
    .setDutyCycle = PWM_SetDutyCycle_Impl,
    .setFrequency = PWM_SetFrequency_Impl,
    .setConfig = PWM_SetConfig_Impl,
    .isEnabled = PWM_IsEnabled_Impl,
    .getDuty = PWM_GetDuty_Impl,
    .getFrequency = PWM_GetFrequency_Impl,
    .getPrescaler = PWM_GetPrescaler_Impl,
    .getPeriod = PWM_GetPeriod_Impl,
};

// GPIO到PWM的映射表（含默认与重映射通道）
// 格式: { port, pin, timer, channel, is_complementary, is_remapped, remap_value }
static const PWM_PinMap pwm_pin_map[] = {
    // TIM1 默认
    {GPIOA, GPIO_Pin_8, TIM1, TIM_Channel_1, false, false, 0},
    {GPIOB, GPIO_Pin_13, TIM1, TIM_Channel_1, true, false, 0},  // CH1N
    {GPIOA, GPIO_Pin_9, TIM1, TIM_Channel_2, false, false, 0},
    {GPIOB, GPIO_Pin_14, TIM1, TIM_Channel_2, true, false, 0},  // CH2N
    {GPIOA, GPIO_Pin_10, TIM1, TIM_Channel_3, false, false, 0},
    {GPIOB, GPIO_Pin_15, TIM1, TIM_Channel_3, true, false, 0},  // CH3N
    {GPIOA, GPIO_Pin_11, TIM1, TIM_Channel_4, false, false, 0},
    // TIM1 部分重映射: CH1N->PA7, CH2N->PB0, CH3N->PB1
    {GPIOA, GPIO_Pin_7, TIM1, TIM_Channel_1, true, true, GPIO_PartialRemap_TIM1},
    {GPIOB, GPIO_Pin_0, TIM1, TIM_Channel_2, true, true, GPIO_PartialRemap_TIM1},
    {GPIOB, GPIO_Pin_1, TIM1, TIM_Channel_3, true, true, GPIO_PartialRemap_TIM1},

    // TIM2 默认: CH1/PA0, CH2/PA1, CH3/PA2, CH4/PA3
    {GPIOA, GPIO_Pin_0, TIM2, TIM_Channel_1, false, false, 0},
    {GPIOA, GPIO_Pin_1, TIM2, TIM_Channel_2, false, false, 0},
    {GPIOA, GPIO_Pin_2, TIM2, TIM_Channel_3, false, false, 0},
    {GPIOA, GPIO_Pin_3, TIM2, TIM_Channel_4, false, false, 0},
    // TIM2 部分重映射1: CH1->PA15, CH2->PB3
    {GPIOA, GPIO_Pin_15, TIM2, TIM_Channel_1, false, true, GPIO_PartialRemap1_TIM2},
    {GPIOB, GPIO_Pin_3, TIM2, TIM_Channel_2, false, true, GPIO_PartialRemap1_TIM2},
    // TIM2 部分重映射2: CH3->PB10, CH4->PB11
    {GPIOB, GPIO_Pin_10, TIM2, TIM_Channel_3, false, true, GPIO_PartialRemap2_TIM2},
    {GPIOB, GPIO_Pin_11, TIM2, TIM_Channel_4, false, true, GPIO_PartialRemap2_TIM2},

    // TIM3 默认: CH1/PA6, CH2/PA7, CH3/PB0, CH4/PB1
    {GPIOA, GPIO_Pin_6, TIM3, TIM_Channel_1, false, false, 0},
    {GPIOA, GPIO_Pin_7, TIM3, TIM_Channel_2, false, false, 0},
    {GPIOB, GPIO_Pin_0, TIM3, TIM_Channel_3, false, false, 0},
    {GPIOB, GPIO_Pin_1, TIM3, TIM_Channel_4, false, false, 0},
    // TIM3 部分重映射: CH1->PB4, CH2->PB5
    {GPIOB, GPIO_Pin_4, TIM3, TIM_Channel_1, false, true, GPIO_PartialRemap_TIM3},
    {GPIOB, GPIO_Pin_5, TIM3, TIM_Channel_2, false, true, GPIO_PartialRemap_TIM3},

    // TIM4 默认: CH1/PB6, CH2/PB7, CH3/PB8, CH4/PB9
    {GPIOB, GPIO_Pin_6, TIM4, TIM_Channel_1, false, false, 0},
    {GPIOB, GPIO_Pin_7, TIM4, TIM_Channel_2, false, false, 0},
    {GPIOB, GPIO_Pin_8, TIM4, TIM_Channel_3, false, false, 0},
    {GPIOB, GPIO_Pin_9, TIM4, TIM_Channel_4, false, false, 0},
#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
    // TIM4 重映射: CH1/CH2/CH3/CH4 -> PD12/PD13/PD14/PD15
    {GPIOD, GPIO_Pin_12, TIM4, TIM_Channel_1, false, true, GPIO_Remap_TIM4},
    {GPIOD, GPIO_Pin_13, TIM4, TIM_Channel_2, false, true, GPIO_Remap_TIM4},
    {GPIOD, GPIO_Pin_14, TIM4, TIM_Channel_3, false, true, GPIO_Remap_TIM4},
    {GPIOD, GPIO_Pin_15, TIM4, TIM_Channel_4, false, true, GPIO_Remap_TIM4},
#endif

#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
    // TIM5 - 仅高密度和XL密度芯片支持
    {GPIOA, GPIO_Pin_0, TIM5, TIM_Channel_1, false, false, 0},
    {GPIOA, GPIO_Pin_1, TIM5, TIM_Channel_2, false, false, 0},
    {GPIOA, GPIO_Pin_2, TIM5, TIM_Channel_3, false, false, 0},
    {GPIOA, GPIO_Pin_3, TIM5, TIM_Channel_4, false, false, 0},

    // TIM8 - 仅高密度和XL密度芯片支持（PC6-PC9 优先匹配 TIM8）
    {GPIOC, GPIO_Pin_6, TIM8, TIM_Channel_1, false, false, 0},
    {GPIOC, GPIO_Pin_7, TIM8, TIM_Channel_2, false, false, 0},
    {GPIOC, GPIO_Pin_8, TIM8, TIM_Channel_3, false, false, 0},
    {GPIOC, GPIO_Pin_9, TIM8, TIM_Channel_4, false, false, 0},

    // TIM3 完全重映射: CH1/CH2/CH3/CH4 -> PC6/PC7/PC8/PC9（与 TIM8 同引脚，表中排在 TIM8 之后）
    {GPIOC, GPIO_Pin_6, TIM3, TIM_Channel_1, false, true, GPIO_FullRemap_TIM3},
    {GPIOC, GPIO_Pin_7, TIM3, TIM_Channel_2, false, true, GPIO_FullRemap_TIM3},
    {GPIOC, GPIO_Pin_8, TIM3, TIM_Channel_3, false, true, GPIO_FullRemap_TIM3},
    {GPIOC, GPIO_Pin_9, TIM3, TIM_Channel_4, false, true, GPIO_FullRemap_TIM3},
#endif

    {NULL, 0, NULL, 0, false, false, 0}  // 结束标记
};

// 根据GPIO查找映射配置
static const PWM_PinMap *PWM_FindPinMap(GPIO_TypeDef *gpio_port, uint16_t gpio_pin)
{
	for (int i = 0; pwm_pin_map[i].gpio_port != NULL; i++)
	{
		if (pwm_pin_map[i].gpio_port == gpio_port &&
		    pwm_pin_map[i].gpio_pin == gpio_pin)
		{
			return &pwm_pin_map[i];
		}
	}
	return NULL;
}

// 创建PWM驱动
PWM_Handle_t *PWM_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin)
{
	const PWM_PinMap *pin_map = PWM_FindPinMap(gpio_port, gpio_pin);
	if (pin_map == NULL)
	{
		return NULL;
	}

	PWM_Handle_t *driver = (PWM_Handle_t *)malloc(sizeof(PWM_Handle_t));
	if (!driver)
		return NULL;

	// 初始化驱动结构体
	driver->timer = pin_map->timer;
	driver->channel = pin_map->channel;
	driver->frequency = 1000;
	driver->duty_cycle = 0;
	driver->enabled = false;
	driver->gpio_port = gpio_port;
	driver->gpio_pin = gpio_pin;
	driver->is_complementary = pin_map->is_complementary;

	// 初始化硬件
	PWM_InitHardware(driver, pin_map);
	PWM_Start_Impl(driver);  // 启动PWM输出

	return driver;
}

// 销毁PWM驱动
void PWM_Destroy(PWM_Handle_t *driver)
{
	if (driver)
	{
		PWM_Stop_Impl(driver);
		free(driver);
	}
}

// 获取PWM操作接口
inline PWM_Operations *PWM_GetOperations(void)
{
	return &pwm_ops;
}

// 初始化硬件
static void PWM_InitHardware(PWM_Handle_t *driver, const PWM_PinMap *pin_map)
{
	GPIO_InitTypeDef        GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef       TIM_OCInitStructure;

	// 启用GPIO时钟
	if (pin_map->gpio_port == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (pin_map->gpio_port == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
	else if (pin_map->gpio_port == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if (pin_map->gpio_port == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}
#endif

	// 重映射通道：使能 AFIO 并配置对应重映射
	if (pin_map->is_remapped)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
		GPIO_PinRemapConfig(pin_map->remap_value, ENABLE);
	}

	// 配置GPIO为复用推挽输出
	GPIO_InitStructure.GPIO_Pin = pin_map->gpio_pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(pin_map->gpio_port, &GPIO_InitStructure);

	// 启用定时器时钟
	if (pin_map->timer == TIM1)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	}
	else if (pin_map->timer == TIM2)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	}
	else if (pin_map->timer == TIM3)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	}
	else if (pin_map->timer == TIM4)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	}
#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
	else if (pin_map->timer == TIM5)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	}
	else if (pin_map->timer == TIM8)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
	}
#endif

	// 配置定时器时基
	TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	// 高级定时器需要设置重复计数器
	if (driver->timer == TIM1 || driver->timer == TIM8)
	{
		TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	}
	TIM_TimeBaseInit(driver->timer, &TIM_TimeBaseStructure);

	// 配置PWM模式
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;

	// 配置互补通道（仅高级定时器TIM1/TIM8支持）
	if (driver->is_complementary && (driver->timer == TIM1 || driver->timer == TIM8))
	{
		TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
		TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
		TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	}
	else
	{
		TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
		TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
		TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	}

	switch (driver->channel)
	{
	case TIM_Channel_1:
		TIM_OC1Init(driver->timer, &TIM_OCInitStructure);
		TIM_OC1PreloadConfig(driver->timer, TIM_OCPreload_Enable);
		break;
	case TIM_Channel_2:
		TIM_OC2Init(driver->timer, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(driver->timer, TIM_OCPreload_Enable);
		break;
	case TIM_Channel_3:
		TIM_OC3Init(driver->timer, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(driver->timer, TIM_OCPreload_Enable);
		break;
	case TIM_Channel_4:
		TIM_OC4Init(driver->timer, &TIM_OCInitStructure);
		TIM_OC4PreloadConfig(driver->timer, TIM_OCPreload_Enable);
		break;
	default:
		break;
	}

	TIM_ARRPreloadConfig(driver->timer, ENABLE);

	// 配置死区时间（仅高级定时器TIM1/TIM8，且使用互补通道时）
	if ((driver->timer == TIM1 || driver->timer == TIM8) && driver->is_complementary)
	{
		TIM_BDTRInitTypeDef TIM_BDTRInitStructure;
		TIM_BDTRStructInit(&TIM_BDTRInitStructure);
		TIM_BDTRInitStructure.TIM_DeadTime = 0x10;
		TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;
		TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;
		TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
		TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
		TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
		TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF;
		TIM_BDTRConfig(driver->timer, &TIM_BDTRInitStructure);
	}
}

// 静态操作函数实现
static void PWM_Start_Impl(PWM_Handle_t *driver)
{
	assert_param(driver != NULL);

	driver->enabled = true;

	// 启用定时器（如果尚未启用）
	if (!(driver->timer->CR1 & TIM_CR1_CEN))
	{
		TIM_Cmd(driver->timer, ENABLE);
	}

	// 启用对应通道的输出
	TIM_CCxCmd(driver->timer, driver->channel, TIM_CCx_Enable);

	// 如果使用互补通道，启用互补通道输出
	if (driver->is_complementary && (driver->timer == TIM1 || driver->timer == TIM8))
	{
		TIM_CCxNCmd(driver->timer, driver->channel, TIM_CCxN_Enable);
	}

	// 对于高级定时器（TIM1/TIM8），需要使能主输出
	if (driver->timer == TIM1 || driver->timer == TIM8)
	{
		TIM_CtrlPWMOutputs(driver->timer, ENABLE);
	}
}

static void PWM_Stop_Impl(PWM_Handle_t *driver)
{
	assert_param(driver != NULL);

	driver->enabled = false;

	// 禁用对应通道的输出
	TIM_CCxCmd(driver->timer, driver->channel, TIM_CCx_Disable);

	// 如果使用互补通道，禁用互补通道输出
	if (driver->is_complementary && (driver->timer == TIM1 || driver->timer == TIM8))
	{
		TIM_CCxNCmd(driver->timer, driver->channel, TIM_CCxN_Disable);
	}

	// TIM1和TIM8是高级定时器，需要禁用主输出才能停止PWM
	if (driver->timer == TIM1 || driver->timer == TIM8)
	{
		TIM_CtrlPWMOutputs(driver->timer, DISABLE);
	}
	// 注意：这里不禁用定时器本身，因为可能有其他通道在使用
	// 定时器继续运行，只是该通道没有输出
}

static void PWM_SetDutyCycle_Impl(PWM_Handle_t *driver, float duty_cycle)
{
	assert_param(driver != NULL);

	driver->duty_cycle = duty_cycle;

	// 对于互补通道，反转占空比，使得设置100%时背光完全亮（与正相PWM行为一致）
	float actual_duty_cycle = duty_cycle;
	if (driver->is_complementary && (driver->timer == TIM1 || driver->timer == TIM8))
	{
		actual_duty_cycle = 100.0f - duty_cycle;
	}

	uint16_t compare_value = (driver->timer->ARR * actual_duty_cycle) / 100;
	switch (driver->channel)
	{
	case TIM_Channel_1:
		driver->timer->CCR1 = compare_value;
		break;
	case TIM_Channel_2:
		driver->timer->CCR2 = compare_value;
		break;
	case TIM_Channel_3:
		driver->timer->CCR3 = compare_value;
		break;
	case TIM_Channel_4:
		driver->timer->CCR4 = compare_value;
		break;
	default:
		break;
	}
}

/**
 * @brief 设置指定频率 根据频率计算预分频器和周期值
 * @param driver 驱动指针
 * @param frequency 频率
 * @note 频率 = 72MHz / (prescaler + 1) / (period + 1)
 * @note prescaler = (timer_clock / (frequency * 1000)) - 1
 * @note period = 1000 - 1
 @return 无
 */
static void PWM_SetFrequency_Impl(PWM_Handle_t *driver, uint32_t frequency)
{
	assert_param(driver != NULL);

	driver->frequency = frequency;
	RCC_ClocksTypeDef rcc_clocks;
	RCC_GetClocksFreq(&rcc_clocks);

	uint32_t timer_clock;
	// 计算定时器实际时钟频率
	// STM32规则：当APB分频器 > 1时，定时器时钟 = APB时钟 × 2
	if (driver->timer == TIM1 || driver->timer == TIM8)
	{
		// APB2定时器：检查PPRE2[2]位（第13位）判断分频器是否 > 1
		uint32_t pclk2 = rcc_clocks.PCLK2_Frequency;
		timer_clock = (RCC->CFGR & (1 << 13)) ? (pclk2 * 2) : pclk2;
	}
	else
	{
		// APB1定时器：检查PPRE1[2]位（第10位）判断分频器是否 > 1
		uint32_t pclk1 = rcc_clocks.PCLK1_Frequency;
		timer_clock = (RCC->CFGR & (1 << 10)) ? (pclk1 * 2) : pclk1;
	}

	uint32_t prescaler = (timer_clock / (frequency * 1000)) - 1;
	if (prescaler > 0xFFFF)
	{
		prescaler = 0xFFFF;
	}

	driver->timer->PSC = prescaler;
	driver->timer->ARR = 1000 - 1;
}

static void PWM_SetConfig_Impl(PWM_Handle_t *driver, uint32_t prescaler, uint32_t period)
{
	assert_param(driver != NULL);

	// 停止定时器以安全修改配置
	TIM_Cmd(driver->timer, DISABLE);

	// 更新预分频器、周期值和频率
	driver->timer->PSC = prescaler;
	driver->timer->ARR = period;

	RCC_ClocksTypeDef rcc_clocks;
	RCC_GetClocksFreq(&rcc_clocks);
	uint32_t timer_clock;

	// 计算定时器实际时钟频率
	// STM32规则：当APB分频器 > 1时，定时器时钟 = APB时钟 × 2
	if (driver->timer == TIM1 || driver->timer == TIM8)
	{
		// APB2定时器：检查PPRE2[2]位（第13位）判断分频器是否 > 1
		uint32_t pclk2 = rcc_clocks.PCLK2_Frequency;
		timer_clock = (RCC->CFGR & (1 << 13)) ? (pclk2 * 2) : pclk2;
	}
	else
	{
		// APB1定时器：检查PPRE1[2]位（第10位）判断分频器是否 > 1
		uint32_t pclk1 = rcc_clocks.PCLK1_Frequency;
		timer_clock = (RCC->CFGR & (1 << 10)) ? (pclk1 * 2) : pclk1;
	}

	driver->frequency = timer_clock / (prescaler + 1) / (period + 1);

	// 重新计算占空比以保持相对值
	// 对于互补通道，反转占空比，使得设置100%时背光完全亮（与正相PWM行为一致）
	float actual_duty_cycle = driver->duty_cycle;
	if (driver->is_complementary && (driver->timer == TIM1 || driver->timer == TIM8))
	{
		actual_duty_cycle = 100.0f - driver->duty_cycle;
	}

	uint16_t compare_value = (period * actual_duty_cycle) / 100;

	switch (driver->channel)
	{
	case TIM_Channel_1:
		driver->timer->CCR1 = compare_value;
		break;
	case TIM_Channel_2:
		driver->timer->CCR2 = compare_value;
		break;
	case TIM_Channel_3:
		driver->timer->CCR3 = compare_value;
		break;
	case TIM_Channel_4:
		driver->timer->CCR4 = compare_value;
		break;
	default:
		break;
	}

	// 如果之前是启用状态，重新启动
	if (driver->enabled)
	{
		TIM_Cmd(driver->timer, ENABLE);
	}
}

static bool PWM_IsEnabled_Impl(PWM_Handle_t *driver)
{
	assert_param(driver != NULL);
	return driver->enabled;
}

static uint16_t PWM_GetDuty_Impl(PWM_Handle_t *driver)
{
	assert_param(driver != NULL);
	return driver->duty_cycle;
}

static uint32_t PWM_GetFrequency_Impl(PWM_Handle_t *driver)
{
	assert_param(driver != NULL);
	return driver->frequency;
}

static uint32_t PWM_GetPrescaler_Impl(PWM_Handle_t *driver)
{
	assert_param(driver != NULL);
	return driver->timer->PSC + 1;
}

static uint32_t PWM_GetPeriod_Impl(PWM_Handle_t *driver)
{
	assert_param(driver != NULL);
	return driver->timer->ARR + 1;
}