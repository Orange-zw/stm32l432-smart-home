#include "stm32f10x.h" // Device header
#include "delay.h"     // Device header
#include "HX1838.h"    // 包含头文件以使用宏定义
#include "oled.h"

volatile u32 hw_code;   // 定义一个32位数据变量，保存接收码
volatile u8 hw_rx_flag; // 定义一个8位数据的变量，用于指示接收标志

/*
 *==============================================================================
 *函数名称：HX1838_Init
 *函数功能：初始化红外遥控模块
 *输入参数：无
 *返回值：无
 *备  注：红外端口初始化函数，时钟端口及外部中断初始化
 *==============================================================================
 */
void HX1838_Init(void)
{
    // 结构体定义
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 开启时钟
    RCC_APB2PeriphClockCmd(HX1838_GPIO_CLK | HX1838_AFIO_CLK, ENABLE);

    // 配置GPIO结构体
    GPIO_InitStructure.GPIO_Pin = HX1838_GPIO_PIN; // 红外接收
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入模式
    GPIO_Init(HX1838_GPIO_PORT, &GPIO_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, HX1838_GPIO_PIN_SOURCE); // 选择GPIO管脚用作外部中断线路
    EXTI_ClearITPendingBit(HX1838_EXTI_LINE);

    // 配置外部中断
    EXTI_InitStructure.EXTI_Line = HX1838_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // 配置NVIC结构体
    NVIC_InitStructure.NVIC_IRQChannel = HX1838_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HX1838_IRQ_PREEMPT_PRIORITY; // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HX1838_IRQ_SUB_PRIORITY;            // 响应优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                                     // 使能
    NVIC_Init(&NVIC_InitStructure);
}

/*
 *==============================================================================
 *函数名称：HX1838_RecordHightTime
 *函数功能：记录高电平持续时间并返回
 *输入参数：无
 *返回值：t：高电平持续时间
 *备  注：使用循环计数延时，不依赖SysTick，可在中断中安全使用
 *==============================================================================
 */
u8 HX1838_RecordHightTime(void)
{
    u8 t = 0;
    volatile uint32_t delay_count;
    
    while (GPIO_ReadInputDataBit(HX1838_GPIO_PORT, HX1838_GPIO_PIN) == 1) // 高电平
    {
        // 使用循环计数延时约20us（72MHz时钟，约1440个周期）
        // 注意：实际延时时间需要根据CPU频率调整
        for (delay_count = 0; delay_count < 144; delay_count++)
        {
            __NOP(); // 空操作，增加延时精度
        }
        
        t++;
        if (t >= 250) {
            return t; // 超时溢出
        }
    }
    return t;
}

/*
 *==============================================================================
 *函数名称：Med_Hw_ReadKeyValue
 *函数功能：读取键值
 *输入参数：无
 *返回值：红外键值
 *备  注：每一个键值是测试出来的，不同遥控器键值可能不同
 *==============================================================================
 */
u8 HX1838_ReadKeyValue(void)
{
    u8 keyValue = HX1838_KEY_INVALID; // 默认返回无效值

    if (hw_code == 0x00FF9867) {
        keyValue = 0;
    } else if (hw_code == 0x00FFA25D) {
        keyValue = 1;
    } else if (hw_code == 0x00FF629D) {
        keyValue = 2;
    } else if (hw_code == 0x00FFE21D) {
        keyValue = 3;
    } else if (hw_code == 0x00FF22DD) {
        keyValue = 4;
    } else if (hw_code == 0x00FF02FD) {
        keyValue = 5;
    } else if (hw_code == 0x00FFC23D) {
        keyValue = 6;
    } else if (hw_code == 0x00FFE01F) {
        keyValue = 7;
    } else if (hw_code == 0x00FFA857) {
        keyValue = 8;
    } else if (hw_code == 0x00FF906F) {
        keyValue = 9;
    } else if (hw_code == 0x00FF6897) // 按键*按下
    {
        keyValue = 10;
    } else if (hw_code == 0x00FFB04F) // 按键#按下
    {
        keyValue = 11;
    } else if (hw_code == 0x00FF38C7) // 按键OK按下
    {
        keyValue = 12;
    } else if (hw_code == 0x00FF18E7) // 按键"上"按下
    {
        keyValue = 13;
    } else if (hw_code == 0x00FF4AB5) // 按键"下"按下
    {
        keyValue = 14;
    } else if (hw_code == 0x00FF10EF) // 按键"左"按下
    {
        keyValue = 15;
    } else if (hw_code == 0x00FF5AA5) // 按键"右"按下
    {
        keyValue = 16;
    }

    // 如果接收到有效按键，自动清除接收标志位和接收码
    if (keyValue != HX1838_KEY_INVALID) {
        hw_rx_flag = 0; // 清除接收完成标志位
        hw_code = 0;    // 清零接收码
    }
    return keyValue;
}

/*
 *==============================================================================
 *函数名称：HX1838_IRQ_HANDLER
 *函数功能：外部中断服务函数
 *输入参数：无
 *返回值：无
 *备  注：无
 *==============================================================================
 */
void HX1838_IRQ_HANDLER(void) // 红外遥控外部中断
{
    u8 Tim = 0, Ok = 0, Data, Num = 0;

    while (1) {
        if (GPIO_ReadInputDataBit(HX1838_GPIO_PORT, HX1838_GPIO_PIN) == 1) {
            Tim = HX1838_RecordHightTime(); // 获得此次高电平时间

            if (Tim >= 250) {
                break; // 不是有用的信号
            }

            if (Tim >= 200 && Tim < 250) {
                Ok = 1; // 收到起始信号
            } else if (Tim >= 60 && Tim < 90) {
                Data = 1; // 收到数据 1
            } else if (Tim >= 10 && Tim < 50) {
                Data = 0; // 收到数据 0
            }

            if (Ok == 1) {
                hw_code <<= 1;
                hw_code += Data;

                // 接收完成
                if (Num >= 32) {
                    hw_rx_flag = 1;
                    break;
                }
            }
            Num++;
        }
    }

    // 清除中断标志位
    EXTI_ClearITPendingBit(HX1838_EXTI_LINE);
}
