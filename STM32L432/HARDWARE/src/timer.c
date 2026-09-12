#include "timer.h"

TIM_HandleTypeDef TIM7_Handler;      //定时器句柄 
extern vu16 USART3_RX_STA;

//定时器7初始化
//arr：自动重装值。
//psc：时钟预分频数
void TIM7_Int_Init(u16 arr,u16 psc)
{
		TIM7_Handler.Instance=TIM7;                          //通用定时器3
    TIM7_Handler.Init.Prescaler=psc;                     //分频系数
    TIM7_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;    //向上计数器
    TIM7_Handler.Init.Period=arr;                        //自动装载值
    TIM7_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;//时钟分频因子
    HAL_TIM_Base_Init(&TIM7_Handler);
    
    HAL_TIM_Base_Start_IT(&TIM7_Handler); //使能定时器3和定时器3更新中断：TIM_IT_UPDATE									 
}
//定时器底册驱动，开启时钟，设置中断优先级
//此函数会被HAL_TIM_Base_Init()函数调用
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  if(htim->Instance==TIM7)
	{
		__HAL_RCC_TIM7_CLK_ENABLE();            //使能TIM7时钟
		HAL_NVIC_SetPriority(TIM7_IRQn,1,3);    //设置中断优先级，抢占优先级0，子优先级1
		HAL_NVIC_EnableIRQ(TIM7_IRQn);          //开启ITM7中断   
	}
}
//定时器7中断服务程序		    
void TIM7_IRQHandler(void)
{
	ESP8266_RX_STA|=1<<15;	//标记接收完成
	ESP8266_RX_BUF[ESP8266_RX_STA&0X7FFF]=0;//添加结束符
	if((ESP8266_STA&7) == 4)  //标记命令接收完成
		{
			ESP8266_STA&=~7;
			ESP8266_STA|=5;
		}
	__HAL_TIM_CLEAR_FLAG(&TIM7_Handler,TIM_EventSource_Update );       //清除TIM7更新中断标志  
	TIM7->CR1&=~(1<<0);     			//关闭定时器7    
} 

