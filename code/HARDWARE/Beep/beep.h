#ifndef __BEEP_SENSOR_H__
#define __BEEP_SENSOR_H__

#include "DO_sensor.h"
#include <stdint.h>

// 蜂鸣器操作函数指针类型
typedef void (*Beep_BeepFunc)(DO_handle_t beep, int duration_ms);

// 蜂鸣器接口结构体
typedef struct
{
    DO_handle_t base; // 基类
    Beep_BeepFunc beep;
} BeepSensor;

// 工厂函数 - 创建蜂鸣器
BeepSensor *BeepSensor_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin, int active_level);

// 销毁函数
void BeepSensor_Destroy(BeepSensor *beep);

// 初始化函数
void BeepSensor_Init(BeepSensor *beep);

#endif /* __BEEP_SENSOR_H__ */
