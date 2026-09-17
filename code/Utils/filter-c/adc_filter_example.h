/*
 * ADC滤波器头文件
 * 用于平滑ADC传感器数据采集
 */

#ifndef ADC_FILTER_H
#define ADC_FILTER_H

#include <stdint.h>

// ADC滤波器句柄（不透明指针）
typedef void* ADC_Filter_Handle;

/**
 * @brief 创建ADC低通滤波器
 * @param sampling_freq 采样频率 (Hz)，例如：10, 50, 100
 * @param cutoff_freq 截止频率 (Hz)，建议为采样频率的1/5到1/10
 * @param order 滤波器阶数，2或4（推荐2）
 * @return 滤波器句柄，失败返回NULL
 */
ADC_Filter_Handle ADC_Filter_Create(uint16_t sampling_freq, uint16_t cutoff_freq, uint8_t order);

/**
 * @brief 对ADC值进行滤波处理
 * @param filter 滤波器句柄
 * @param adc_raw_value 原始ADC值 (0-4095 for 12-bit ADC)
 * @return 滤波后的ADC值
 */
uint16_t ADC_Filter_Process(ADC_Filter_Handle filter, uint16_t adc_raw_value);

/**
 * @brief 释放ADC滤波器资源
 * @param filter 滤波器句柄
 */
void ADC_Filter_Destroy(ADC_Filter_Handle filter);

#endif /* ADC_FILTER_H */
