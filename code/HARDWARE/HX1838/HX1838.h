#ifndef __HX1838_H
#define __HX1838_H

#include "stm32f10x.h" // Device header

// ==================== 引脚配置区域 ====================
// 在这里修改引脚定义，提高代码可移植性
#define HX1838_GPIO_PORT GPIOA                 // 红外接收器连接的GPIO端口
#define HX1838_GPIO_PIN GPIO_Pin_7             // 红外接收器连接的GPIO引脚
#define HX1838_GPIO_PIN_SOURCE GPIO_PinSource7 // GPIO引脚源
#define HX1838_EXTI_LINE EXTI_Line7            // 外部中断线
#define HX1838_EXTI_IRQn EXTI9_5_IRQn            // 外部中断IRQ通道
#define HX1838_GPIO_CLK RCC_APB2Periph_GPIOA   // GPIO时钟
#define HX1838_AFIO_CLK RCC_APB2Periph_AFIO    // AFIO时钟

// 中断优先级配置
// 注意：不应设置为最高优先级（0），否则会阻塞SysTick等系统中断
// 建议设置为1或2，确保SysTick能正常更新
#define HX1838_IRQ_PREEMPT_PRIORITY 1 // 抢占优先级（建议设置为1或2，不要设置为0）
#define HX1838_IRQ_SUB_PRIORITY 1     // 响应优先级

// 中断处理函数宏定义
#define HX1838_IRQ_HANDLER EXTI9_5_IRQHandler // 中断处理函数名

// ==================== 常量定义 ====================
#define HX1838_KEY_INVALID 17 // 无效按键值

// ==================== 函数声明 ====================
void HX1838_Init(void);
u8 HX1838_RecordHightTime(void);
u8 HX1838_ReadKeyValue(void);

#endif
