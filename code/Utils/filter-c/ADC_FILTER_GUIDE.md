# ADC数据滤波使用指南

## 快速开始

### 1. 基本使用

```c
#include "adc_filter_example.h"

// 创建滤波器
ADC_Filter *filter = ADC_Filter_Create(10, 2, 2);
// 参数：采样频率10Hz，截止频率2Hz，2阶滤波器

// 在主循环中使用
while (1) {
    uint16_t adc_raw = Get_Adc(0);              // 读取原始ADC值
    uint16_t adc_filtered = ADC_Filter_Process(filter, adc_raw);  // 滤波
    // 使用滤波后的值 adc_filtered
    delay_ms(100);  // 对应10Hz采样率
}

// 程序结束前释放
ADC_Filter_Destroy(filter);
```

## 参数选择

### 采样频率 (sampling_freq)

根据你的实际采样间隔计算：
- 每 100ms 采样一次 = **10 Hz**
- 每 50ms 采样一次 = **20 Hz**
- 每 10ms 采样一次 = **100 Hz**
- 每 1ms 采样一次 = **1000 Hz**

### 截止频率 (cutoff_freq)

**经验公式：截止频率 = 采样频率 × (1/5 到 1/10)**

| 传感器类型 | 变化速度 | 推荐比例 | 示例（采样率10Hz） |
|----------|---------|---------|------------------|
| 温度、湿度 | 很慢 | 1/10 | 1 Hz |
| 压力、光照 | 中等 | 1/5 | 2 Hz |
| 加速度、振动 | 快 | 1/3 | 3 Hz |

**原则：**
- 需要更平滑 → 减小截止频率（响应变慢）
- 需要更快响应 → 增大截止频率（平滑度降低）

### 滤波器阶数 (order)

- **2阶（推荐）**：计算量小，效果足够，响应较快
- **4阶**：效果更好，但计算量大，响应较慢

对于ADC数据平滑，**2阶通常足够**。

## 实际应用示例

### 示例1：温度传感器（缓慢变化）

```c
// 每200ms采样一次（5Hz），需要平滑缓慢变化
ADC_Filter *temp_filter = ADC_Filter_Create(5, 0.5, 2);
// 采样频率5Hz，截止频率0.5Hz，2阶
```

### 示例2：光照传感器（中等速度）

```c
// 每50ms采样一次（20Hz）
ADC_Filter *light_filter = ADC_Filter_Create(20, 4, 2);
// 采样频率20Hz，截止频率4Hz，2阶
```

### 示例3：压力传感器（快速变化）

```c
// 每10ms采样一次（100Hz）
ADC_Filter *pressure_filter = ADC_Filter_Create(100, 20, 2);
// 采样频率100Hz，截止频率20Hz，2阶
```

### 示例4：多通道ADC

```c
// 为每个通道创建独立的滤波器
ADC_Filter *ch0_filter = ADC_Filter_Create(10, 2, 2);
ADC_Filter *ch1_filter = ADC_Filter_Create(10, 2, 2);
ADC_Filter *ch2_filter = ADC_Filter_Create(10, 2, 2);

while (1) {
    uint16_t ch0 = ADC_Filter_Process(ch0_filter, Get_Adc(0));
    uint16_t ch1 = ADC_Filter_Process(ch1_filter, Get_Adc(1));
    uint16_t ch2 = ADC_Filter_Process(ch2_filter, Get_Adc(2));
    delay_ms(100);
}
```

## 集成到现有代码

### 方案A：直接替换（最简单）

```c
// 原来的代码：
uint16_t adc_value = Get_Adc(0);

// 改为：
static ADC_Filter *filter = NULL;
if (filter == NULL) {
    filter = ADC_Filter_Create(10, 2, 2);  // 初始化一次
}
uint16_t adc_value = ADC_Filter_Process(filter, Get_Adc(0));
```

### 方案B：封装到函数中

```c
// 在你的ADC头文件中
extern ADC_Filter *adc_filter;

// 在你的ADC源文件中
static ADC_Filter *adc_filter = NULL;

uint16_t Get_Adc_Filtered(uint8_t ch)
{
    if (adc_filter == NULL) {
        adc_filter = ADC_Filter_Create(10, 2, 2);
    }
    return ADC_Filter_Process(adc_filter, Get_Adc(ch));
}
```

## 效果对比

**滤波前：**
```
ADC值: 512, 518, 509, 521, 510, 517, 508, 523, ...
波动: ±10 左右
```

**滤波后（截止频率2Hz，2阶）：**
```
ADC值: 512, 513, 513, 514, 513, 514, 513, 514, ...
波动: ±1 左右（明显更平滑）
```

## 注意事项

1. **内存占用**：每个滤波器实例约占用几十字节RAM
2. **计算开销**：2阶滤波器计算量很小，适合实时处理
3. **初始化延迟**：滤波器需要几个采样周期才能稳定（通常3-5个周期）
4. **数据类型**：确保 `FTR_PRECISION` 定义为 `float`（默认），使用 `double` 会占用更多内存

## 调参技巧

1. **先设置采样频率**：根据你的实际采样率
2. **从保守值开始**：截止频率 = 采样频率 / 10
3. **观察效果**：如果太平滑（响应慢），增大截止频率
4. **如果还有噪声**：减小截止频率，或增加滤波器阶数到4
