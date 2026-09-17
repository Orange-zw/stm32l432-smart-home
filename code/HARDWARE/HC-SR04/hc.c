/* ============================================================
 * 模块     : HC-SR04 超声波测距驱动
 * 功能     : 触发-回响时序测量与距离换算
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "hc.h"

#include "sys.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "delay.h"
// #include "systick.h"
// 超声波硬件接口定义

u16 msHcCount = 0;
// 定时器4设置
void hcsr04_NVIC()
{
   NVIC_InitTypeDef NVIC_InitStructure;
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

   NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
}

// IO口初始化 及其他初始化
void Hcsr04Init()
{
   TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
   GPIO_InitTypeDef GPIO_InitStructure;
   RCC_APB2PeriphClockCmd(HCSR04_CLK, ENABLE);

   GPIO_InitStructure.GPIO_Pin = HCSR04_TRIG;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
   GPIO_Init(HCSR04_PORT, &GPIO_InitStructure);
   GPIO_ResetBits(HCSR04_PORT, HCSR04_TRIG);

   GPIO_InitStructure.GPIO_Pin = HCSR04_ECHO;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
   GPIO_Init(HCSR04_PORT, &GPIO_InitStructure);
   GPIO_ResetBits(HCSR04_PORT, HCSR04_ECHO);

   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

   TIM_DeInit(TIM4);
   TIM_TimeBaseStructure.TIM_Period = (1000 - 1);
   TIM_TimeBaseStructure.TIM_Prescaler = (72 - 1);
   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
   TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

   TIM_ClearFlag(TIM4, TIM_FLAG_Update);
   TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
   hcsr04_NVIC();
   TIM_Cmd(TIM4, DISABLE);
}

// 打开定时器4
static void OpenTimerForHc()
{
   TIM_SetCounter(TIM4, 0);
   msHcCount = 0;
   TIM_Cmd(TIM4, ENABLE);
}

// 关闭定时器4
static void CloseTimerForHc()
{
   TIM_Cmd(TIM4, DISABLE);
}

// 旧版HC-SR04定时器中断实现（已被TIM_IC驱动接管）
void HC_TIM4_IRQHandler_Legacy(void)
{
   if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
   {
      TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
      msHcCount++;
   }
}

// 获取定时器4计数器值
u32 GetEchoTimer(void)
{
   u32 t = 0;
   t = msHcCount * 1000;
   t += TIM_GetCounter(TIM4);
   TIM4->CNT = 0;
   delay_ms(5);
   return t;
}

// 通过定时器4计数器值推算距离
float Hcsr04GetLength(void)
{
   u32 t = 0;
   int i = 0;
   float lengthTemp = 0;
   float sum = 0;
   uint32_t cnt = 0;
   while (i != 5)
   {
      TRIG_Send = 1;
      delay_us(20);
      TRIG_Send = 0;
      while (ECHO_Reci == 0)
      {
         if (cnt++ > 65530)
         {
            // goto end;
            return -1;
         }
      }

      OpenTimerForHc();
      i = i + 1;
      while (ECHO_Reci == 1)
         ;
      CloseTimerForHc();
      t = GetEchoTimer();
      lengthTemp = ((float)t / 58.0); // cm
      sum = lengthTemp + sum;
   }
   lengthTemp = sum / 5.0;
   return lengthTemp;
}

// float Hcsr04GetLength(void)
// {
//    u32 t = 0;
//    int i = 0;
//    float lengthTemp = 0;
//    float sum = 0;
//    uint16_t cnt = 0;
//    // while (i != 5)
//    // {
//    TRIG_Send = 1;
//    delay_us(20);
//    TRIG_Send = 0;
//    while (ECHO_Reci == 0)
//    {
//       if (cnt++ > 65530)
//       {
//          // goto end;
//          return -1;
//       }
//    }
//    cnt = 0;
//    OpenTimerForHc();
//    while (ECHO_Reci == 1)
//    {
//       if (cnt++ > 65530)
//       {
//          CloseTimerForHc();
//          // goto end;
//          return -1;
//       }
//    }
//    CloseTimerForHc();
//    t = GetEchoTimer();
//    lengthTemp = ((float)t / 58.0); // cm
//    sum = lengthTemp + sum;
//    // end:
//    //    i = i + 1;
//    // }
//    // lengthTemp = sum / 5.0;
//    lengthTemp = sum;
//    return lengthTemp;
// }
