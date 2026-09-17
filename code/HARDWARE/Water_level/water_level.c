#include "water_level.h"
#include <stdlib.h>
#include <string.h>

// 水位传感器操作函数
static uint16_t WaterLevelSensor_GetLevel_Impl(WaterLevelSensorDevice *sensor)
{
    if (sensor == NULL || sensor->base == NULL)
    {
        return 0;
    }

    uint32_t val = AI_OP(sensor->base, get_raw_value);
    uint16_t level;

    for (level = 0; level < 10; level++)
    {
        if (val <= sensor->levels[level])
            break;
    }
    return level * 10; // 返回百分比
}

// 工厂函数 - 创建水位传感器
WaterLevelSensor *WaterLevelSensor_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin,
                                          ADC_TypeDef *adc_instance, uint8_t adc_channel)
{
    (void)adc_channel;
    WaterLevelSensor *device = (WaterLevelSensor *)malloc(sizeof(WaterLevelSensor));
    if (!device)
        return NULL;

    device->sensor = (WaterLevelSensorDevice *)malloc(sizeof(WaterLevelSensorDevice));
    if (!device->sensor)
    {
        free(device);
        return NULL;
    }

    // 创建基类传感器
    device->sensor->base = AI_Create(gpio_port, gpio_pin, adc_instance);
    if (!device->sensor->base)
    {
        free(device->sensor);
        free(device);
        return NULL;
    }

    // 设置默认水位阈值
    uint16_t default_levels[10] = {60, 100, 150, 220, 250, 320, 420, 520, 850, 1500};
    memcpy(device->sensor->levels, default_levels, sizeof(default_levels));

    // 设置操作函数
    device->getRawValue = AI_GetOperations()->get_raw_value;
    device->getLevel = WaterLevelSensor_GetLevel_Impl;

    WaterLevelSensor_Init(device);

    return device;
}

// 销毁函数
void WaterLevelSensor_Destroy(WaterLevelSensor *sensor)
{
    if (sensor)
    {
        if (sensor->sensor)
        {
            if (sensor->sensor->base)
            {
                AI_Destroy(sensor->sensor->base);
            }
            free(sensor->sensor);
        }
        free(sensor);
    }
}

// 初始化函数
void WaterLevelSensor_Init(WaterLevelSensor *sensor)
{
    (void)sensor;
}