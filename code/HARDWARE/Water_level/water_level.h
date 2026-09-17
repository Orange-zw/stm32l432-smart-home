#ifndef __WATER_LEVEL_SENSOR_H__
#define __WATER_LEVEL_SENSOR_H__

#include "AI_sensor.h"
#include <stdint.h>

// 水位传感器结构体
typedef struct
{
    AI_handle_t base;   // 基类
    uint16_t    levels[10];  // 水位阈值
} WaterLevelSensorDevice;

// 水位传感器操作函数指针类型
typedef uint16_t (*WaterLevelSensor_GetRawValueFunc)(AI_handle_t sensor);
typedef uint16_t (*WaterLevelSensor_GetLevelFunc)(WaterLevelSensorDevice *sensor);

// 水位传感器接口结构体
typedef struct
{
    WaterLevelSensorDevice *sensor;
    WaterLevelSensor_GetRawValueFunc getRawValue;
    WaterLevelSensor_GetLevelFunc getLevel;
} WaterLevelSensor;

// 工厂函数 - 创建水位传感器
WaterLevelSensor *WaterLevelSensor_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin,
                                          ADC_TypeDef *adc_instance, uint8_t adc_channel);

// 销毁函数
void WaterLevelSensor_Destroy(WaterLevelSensor *sensor);

// 初始化函数
void WaterLevelSensor_Init(WaterLevelSensor *sensor);

#endif /* __WATER_LEVEL_SENSOR_H__ */