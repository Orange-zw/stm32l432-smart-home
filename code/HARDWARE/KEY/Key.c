/* ============================================================
 * 模块     : 按键驱动
 * 功能     : 4 路独立按键初始化与带去抖的扫描
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "Key.h"
#include "delay.h"
#include "stm32f10x.h"  // Device header

void Key_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// 使能GPIOA和GPIOB时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	// 禁用JTAG，使能SWD，这样PB3和PB4才能用作普通IO
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	// 配置PB3、PB4为上拉输入
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_5 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// 配置PA12、PA15为上拉输入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// 按键处理函数
// 返回按键值
// mode:0,不支持连续按;1,支持连续按;
// 0，没有任何按键按下
// 1，KEY0按下
// 2，KEY1按下
// 3，KEY2按下
// 4，KEY3按下 WK_UP
// 注意此函数有响应优先级,KEY0>KEY1>KEY2>KEY3!!
u8 KEY_Scan(u8 mode)
{
	static u8 key_up = 1;  // 按键按松开标志
	if (mode)
		key_up = 1;  // 支持长按
	if (key_up && (KEY1 == 0 || KEY2 == 0 || KEY3 == 0 || KEY4 == 0))
	{
		delay_ms(10);  // 去抖动
		key_up = 0;
		if (KEY1 == 0)
			return KEY1_PRES;
		else if (KEY2 == 0)
			return KEY2_PRES;
		else if (KEY3 == 0)
			return KEY3_PRES;
		else if (KEY4 == 0)
			return KEY4_PRES;
	}
	else if (KEY1 == 1 && KEY2 == 1 && KEY3 == 1 && KEY4 == 1)
		key_up = 1;
	return 0;  // 无按键按下
}
