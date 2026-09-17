/*
 * ADC滤波器使用示例
 * 用于平滑ADC传感器数据采集
 */

#include "filter.h"
#include <stdint.h>

// ============ 方案一：简单的低通滤波（推荐） ============

#include "adc_filter_example.h"
#include "filter.h"
#include <stdlib.h>

/**
 * @brief ADC数据平滑滤波器结构体
 */
typedef struct {
    BWLowPass *filter;           // 低通滤波器
    uint16_t sampling_freq;      // 采样频率 (Hz)
    uint16_t cutoff_freq;        // 截止频率 (Hz)
    uint8_t order;               // 滤波器阶数 (2或4)
} ADC_Filter;

/**
 * @brief 创建ADC滤波器
 * @param sampling_freq 采样频率 (Hz)，例如：10, 50, 100
 * @param cutoff_freq 截止频率 (Hz)，建议为采样频率的1/5到1/10
 * @param order 滤波器阶数，2或4（推荐2，计算量小）
 * @return 滤波器句柄，失败返回NULL
 */
ADC_Filter_Handle ADC_Filter_Create(uint16_t sampling_freq, uint16_t cutoff_freq, uint8_t order)
{
    ADC_Filter *adc_filter = (ADC_Filter*)malloc(sizeof(ADC_Filter));
    if (adc_filter == NULL) {
        return NULL;
    }
    
    adc_filter->sampling_freq = sampling_freq;
    adc_filter->cutoff_freq = cutoff_freq;
    adc_filter->order = order;
    
    // 创建Butterworth低通滤波器
    adc_filter->filter = create_bw_low_pass_filter(
        order,                      // 阶数（2或4）
        (FTR_PRECISION)sampling_freq, // 采样频率
        (FTR_PRECISION)cutoff_freq    // 截止频率
    );
    
    if (adc_filter->filter == NULL) {
        free(adc_filter);
        return NULL;
    }
    
    return adc_filter;
}

/**
 * @brief 对ADC值进行滤波处理
 * @param filter 滤波器句柄
 * @param adc_raw_value 原始ADC值 (0-4095 for 12-bit ADC)
 * @return 滤波后的ADC值
 */
uint16_t ADC_Filter_Process(ADC_Filter_Handle filter, uint16_t adc_raw_value)
{
    ADC_Filter *adc_filter = (ADC_Filter*)filter;
    if (adc_filter == NULL || adc_filter->filter == NULL) {
        return adc_raw_value;
    }
    
    // 将ADC值转换为浮点数进行滤波
    FTR_PRECISION filtered = bw_low_pass(adc_filter->filter, (FTR_PRECISION)adc_raw_value);
    
    // 转换回整数（四舍五入）
    return (uint16_t)(filtered + 0.5f);
}

/**
 * @brief 释放ADC滤波器资源
 */
void ADC_Filter_Destroy(ADC_Filter_Handle filter)
{
    ADC_Filter *adc_filter = (ADC_Filter*)filter;
    if (adc_filter != NULL) {
        if (adc_filter->filter != NULL) {
            free_bw_low_pass(adc_filter->filter);
        }
        free(adc_filter);
    }
}

// ============ 使用示例 ============

/*
 * 示例1：基本使用（集成到现有ADC代码中）
 */
void ADC_Filter_Example1(void)
{
    // 创建滤波器
    // 参数说明：
    // - 采样频率：10Hz（假设每100ms采样一次）
    // - 截止频率：2Hz（保留缓慢变化的信号，滤除快速噪声）
    // - 阶数：2（2阶滤波器，计算量小，效果足够）
    ADC_Filter_Handle filter = ADC_Filter_Create(10, 2, 2);
    
    if (filter == NULL) {
        return; // 创建失败
    }
    
    // 在主循环中使用
    while (1) {
        // 读取原始ADC值（使用你现有的ADC函数）
        uint16_t adc_raw = Get_Adc(0);  // 或 AD_GetValue(0), PM25_GetValue() 等
        
        // 滤波处理
        uint16_t adc_filtered = ADC_Filter_Process(filter, adc_raw);
        
        // 使用滤波后的值
        // 例如：显示、存储、控制等
        // printf("Raw: %d, Filtered: %d\n", adc_raw, adc_filtered);
        
        delay_ms(100); // 等待100ms（对应10Hz采样率）
    }
    
    // 程序结束时释放
    ADC_Filter_Destroy(filter);
}

/*
 * 示例2：快速采样场景（例如1kHz采样率）
 */
void ADC_Filter_Example2(void)
{
    // 高采样率时，截止频率可以设置更高
    // 采样频率：1000Hz，截止频率：50Hz
    ADC_Filter_Handle filter = ADC_Filter_Create(1000, 50, 4);
    
    // 使用方式相同
    uint16_t adc_raw = Get_Adc(0);
    uint16_t adc_filtered = ADC_Filter_Process(filter, adc_raw);
    
    ADC_Filter_Destroy(filter);
}

/*
 * 示例3：多通道ADC滤波（每个通道一个滤波器）
 */
#define ADC_CHANNEL_COUNT 3

typedef struct {
    ADC_Filter_Handle filters[ADC_CHANNEL_COUNT];
} MultiChannel_ADC_Filter;

MultiChannel_ADC_Filter* MultiChannel_ADC_Filter_Create(void)
{
    MultiChannel_ADC_Filter *mfilter = (MultiChannel_ADC_Filter*)malloc(sizeof(MultiChannel_ADC_Filter));
    if (mfilter == NULL) {
        return NULL;
    }
    
    // 为每个通道创建滤波器
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        mfilter->filters[i] = ADC_Filter_Create(10, 2, 2);
        if (mfilter->filters[i] == NULL) {
            // 创建失败，清理已创建的滤波器
            for (int j = 0; j < i; j++) {
                ADC_Filter_Destroy(mfilter->filters[j]);
            }
            free(mfilter);
            return NULL;
        }
    }
    
    return mfilter;
}

uint16_t MultiChannel_ADC_Filter_Process(MultiChannel_ADC_Filter *mfilter, uint8_t channel, uint16_t adc_value)
{
    if (mfilter == NULL || channel >= ADC_CHANNEL_COUNT) {
        return adc_value;
    }
    
    return ADC_Filter_Process(mfilter->filters[channel], adc_value);
}

void MultiChannel_ADC_Filter_Destroy(MultiChannel_ADC_Filter *mfilter)
{
    if (mfilter != NULL) {
        for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
            ADC_Filter_Destroy(mfilter->filters[i]);
        }
        free(mfilter);
    }
}

// ============ 参数选择指南 ============

/*
 * 参数选择建议：
 * 
 * 1. 采样频率 (sampling_freq)：
 *    - 根据你的实际采样率设置
 *    - 例如：每100ms采样一次 = 10Hz
 *    - 例如：每10ms采样一次 = 100Hz
 *    - 例如：每1ms采样一次 = 1000Hz
 * 
 * 2. 截止频率 (cutoff_freq)：
 *    - 通常设置为采样频率的 1/5 到 1/10
 *    - 值越小，滤波效果越强（更平滑但响应更慢）
 *    - 值越大，滤波效果越弱（响应更快但平滑度降低）
 *    - 推荐范围：
 *      * 缓慢变化的传感器（温度、湿度）：采样频率的 1/10
 *      * 中等变化的传感器（压力、光照）：采样频率的 1/5
 *      * 快速变化的传感器：采样频率的 1/3
 * 
 * 3. 滤波器阶数 (order)：
 *    - 2阶：计算量小，响应较快，平滑效果适中（推荐）
 *    - 4阶：计算量较大，响应较慢，平滑效果更好
 *    - 对于ADC数据平滑，2阶通常足够
 * 
 * 4. 实际应用示例：
 *    - 温度传感器（采样率10Hz，缓慢变化）：
 *      ADC_Filter_Create(10, 1, 2)   // 截止频率1Hz
 *    
 *    - 光照传感器（采样率50Hz，中等变化）：
 *      ADC_Filter_Create(50, 10, 2)  // 截止频率10Hz
 *    
 *    - 压力传感器（采样率100Hz，快速变化）：
 *      ADC_Filter_Create(100, 20, 2) // 截止频率20Hz
 */
