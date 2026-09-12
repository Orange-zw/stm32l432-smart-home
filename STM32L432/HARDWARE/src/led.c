#include "led.h"
	    
//LED初始化
void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_GPIOB_CLK_ENABLE();           //开启GPIOB时钟
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_Initure.Pin=GPIO_PIN_3|GPIO_PIN_4; 
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
	HAL_GPIO_Init(GPIOB,&GPIO_Initure);

	GPIO_Initure.Pin=GPIO_PIN_8|GPIO_PIN_11; 
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
	HAL_GPIO_Init(GPIOA,&GPIO_Initure);	
}

//手动控制状态：1为对应的电器进行手动控制，0为不进行手动控制，手动控制优先级大于自动控制，只有手动控制和自动控制都关闭时电器才算完全关闭
//本程序只用到空气净化器和抽风机的自动和手动功能，灯的手动功能，其他位为保留位
//位7：空调  位6：空气净化器  位5：抽风机 位4：窗 位3：灯 
u8 Manual_STA;
//自动控制状态  1为对应的电器进行自动控制，0为不自动控制
//位7：空调  位6：空气净化器  位5：抽风机 位4：窗 位3：灯 
u8 AUTO_STA;
//PM2.5质量级别
u8 PM2_5_Quality;

//电器控制函数
void Control()
{
	if(Manual_STA&(1<<6))
	{
		AIR_CLEAR_ON;
	}
	else if(AUTO_STA&(1<<6))
	{
		if(MQ2_Air_Quality>='B' || MQ135_Air_Quality>='B' || PM2_5_Quality>='B')
		{
			AIR_CLEAR_ON;
		}
		else
		{
			AIR_CLEAR_OFF;
		}
	}
	else
	{
		AIR_CLEAR_OFF;
	}
	
	if(Manual_STA&(1<<5))
	{
		EXHAUST_FAN_ON;
	}
	else if(AUTO_STA&(1<<5))
	{
		if(MQ2_Air_Quality>='B' || MQ135_Air_Quality>='B' || PM2_5_Quality>='B')
		{
			EXHAUST_FAN_ON;
		}
		else
		{
			EXHAUST_FAN_OFF;
		}
	}
	else
	{
		EXHAUST_FAN_OFF;
	}
	
	if(Manual_STA&(1<<3))
	{
		LIGHT_ON;
	}
	else
	{
		LIGHT_OFF;
	}
}

