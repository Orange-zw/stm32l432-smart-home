#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

// 参数配置
#define ENCODER_PPR 1320   // 编码器每转脉冲数
#define GEAR_RATIO 120     // 减速比
#define XIAN_CHENG 1       // 线程
#define SAMPLE_TIME_MS 100 // 采样时间（毫秒）

#define R (5.0) // 轮子半径  厘米

// 函数声明
void Encoder_Init(void);        // 初始化编码器和定时器
int32_t Encoder_GetRPM(void);   // 获取当前转速(RPM)
int32_t Encoder_GetCount(void); // 获取编码器原始计数值
void Encoder_setRadius(float r);
void Encoder_ResetCount(void); // 重置编码器计数器
void TIM1_UP_IRQHandler(void); // 声明中断处理函数
float Encoder_GetSpeed(void);

#endif /* __ENCODER_H */
