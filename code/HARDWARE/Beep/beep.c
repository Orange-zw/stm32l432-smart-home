#include "beep.h"
#include <stdlib.h>

// 简单的延时函数
// static void delay_ms(int duration_ms)
// {
//     for (volatile int i = 0; i < duration_ms * 1000; i++)
//         ;
// }

// 蜂鸣器操作函数
static void Beep_Beep_Impl(DO_handle_t beep, int duration_ms)
{
    if (beep == NULL)
        return;

    DO_OP(beep, open);
    delay_ms(duration_ms);
    DO_OP(beep, close);
}

// 工厂函数 - 创建蜂鸣器
BeepSensor *BeepSensor_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, int active_level)
{
    BeepSensor *device = (BeepSensor *)malloc(sizeof(BeepSensor));
    if (!device)
        return NULL;

    device->base = DO_Create(gpio_port, gpio_pin, active_level);
    if (!device->base)
    {
        free(device);
        return NULL;
    }

    // 设置操作函数
    device->beep = Beep_Beep_Impl;

    return device;
}

// 销毁函数
void BeepSensor_Destroy(BeepSensor *beep)
{
    if (beep)
    {
        if (beep->base)
        {
            DO_Destroy(beep->base);
        }
        free(beep);
    }
}

// 初始化函数
void BeepSensor_Init(BeepSensor *beep)
{
    (void)beep;
}