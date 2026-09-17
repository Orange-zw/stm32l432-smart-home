#include "adc.h"

void Adc_Init(u8 pa_index)
{
    // 先初始化IO口
    RCC->APB2ENR |= 1 << 2; // 使能PORTA口时钟
    if (pa_index <= 7)
    {
        GPIOA->CRL &= ~(0xF << (pa_index * 4));
    }
    else if (pa_index <= 15)
    {
        GPIOA->CRH &= ~(0xF << ((pa_index - 8) * 4));
    }
    // 通道10/11设置
    RCC->APB2ENR |= 1 << 9;     // ADC1时钟使能
    RCC->APB2RSTR |= 1 << 9;    // ADC1复位
    RCC->APB2RSTR &= ~(1 << 9); // 复位结束
    RCC->CFGR &= ~(3 << 14);    // 分频因子清零
    // SYSCLK/DIV2=12M ADC时钟设置为12M,ADC最大时钟不能超过14M!
    // 否则将导致ADC准确度下降!
    RCC->CFGR |= 2 << 14;
    ADC1->CR1 &= 0XF0FFFF;  // 工作模式清零
    ADC1->CR1 |= 0 << 16;   // 独立工作模式
    ADC1->CR1 &= ~(1 << 8); // 非扫描模式
    ADC1->CR2 &= ~(1 << 1); // 单次转换模式
    ADC1->CR2 &= ~(7 << 17);
    ADC1->CR2 |= 7 << 17;    // 软件控制转换
    ADC1->CR2 |= 1 << 20;    // 使用用外部触发(SWSTART)!!!	必须使用一个事件来触发
    ADC1->CR2 &= ~(1 << 11); // 右对齐
    ADC1->CR2 |= 1 << 23;    // 使能温度传感器

    ADC1->SQR1 &= ~(0XF << 20);
    ADC1->SQR1 |= 0 << 20; // 1个转换在规则序列中 也就是只转换规则序列1
    // 设置所选通道的采样时间（239.5周期）
    if (pa_index <= 9)
    {
        ADC1->SMPR2 &= ~(7 << (3 * pa_index));
        ADC1->SMPR2 |= (7 << (3 * pa_index));
    }
    else
    {
        ADC1->SMPR1 &= ~(7 << (3 * (pa_index - 10)));
        ADC1->SMPR1 |= (7 << (3 * (pa_index - 10)));
    }

    ADC1->SMPR1 &= ~(7 << 18); // 清除通道16原来的设置
    ADC1->SMPR1 |= 7 << 18;    // 通道16  239.5周期,提高采样时间可以提高精确度

    ADC1->CR2 |= 1 << 0; // 开启AD转换器
    ADC1->CR2 |= 1 << 3; // 使能复位校准
    while (ADC1->CR2 & 1 << 3)
        ; // 等待校准结束
    // 该位由软件设置并由硬件清除。在校准寄存器被初始化后该位将被清除。
    ADC1->CR2 |= 1 << 2; // 开启AD校准
    while (ADC1->CR2 & 1 << 2)
        ; // 等待校准结束
    // 该位由软件设置以开始校准，并在校准结束时由硬件清除
}

// ADC DMA初始化，配置指定PA*通道为DMA采样版本
void ADC_DMA_Init(u8 pa_index)
{
    // 使能GPIOA时钟并配置为模拟输入
    RCC->APB2ENR |= 1 << 2;
    if (pa_index <= 7)
    {
        GPIOA->CRL &= ~(0xF << (pa_index * 4));
    }
    else if (pa_index <= 15)
    {
        GPIOA->CRH &= ~(0xF << ((pa_index - 8) * 4));
    }

    // 使能ADC1时钟并复位
    RCC->APB2ENR |= 1 << 9;
    RCC->APB2RSTR |= 1 << 9;
    RCC->APB2RSTR &= ~(1 << 9);

    // ADC时钟=SYSCLK/2
    RCC->CFGR &= ~(3 << 14);
    RCC->CFGR |= 2 << 14;

    // ADC1基本配置（单次转换，右对齐，软件触发）
    ADC1->CR1 &= 0xF0FFFF;
    ADC1->CR1 |= 0 << 16;   // 独立模式
    ADC1->CR1 &= ~(1 << 8); // 非扫描模式
    ADC1->CR2 &= ~(1 << 1); // 单次转换模式
    ADC1->CR2 &= ~(7 << 17);
    ADC1->CR2 |= 7 << 17;    // 软件触发
    ADC1->CR2 |= 1 << 20;    // 外部触发使能（SWSTART）
    ADC1->CR2 &= ~(1 << 11); // 右对齐
    ADC1->CR2 |= 1 << 23;    // 使能温度传感器

    // 规则序列长度为1
    ADC1->SQR1 &= ~(0xF << 20);
    ADC1->SQR1 |= 0 << 20;

    // 设置采样时间（239.5周期）
    if (pa_index <= 9)
    {
        ADC1->SMPR2 &= ~(7 << (3 * pa_index));
        ADC1->SMPR2 |= (7 << (3 * pa_index));
    }
    else
    {
        ADC1->SMPR1 &= ~(7 << (3 * (pa_index - 10)));
        ADC1->SMPR1 |= (7 << (3 * (pa_index - 10)));
    }

    // 开启ADC并校准
    ADC1->CR2 |= 1 << 0; // ADON
    ADC1->CR2 |= 1 << 3; // 复位校准
    while (ADC1->CR2 & (1 << 3))
        ;
    ADC1->CR2 |= 1 << 2; // 启动校准
    while (ADC1->CR2 & (1 << 2))
        ;

    // 使能DMA
    ADC1->CR2 |= 1 << 8;   // DMA使能
    RCC->AHBENR |= 1 << 0; // 使能DMA1时钟
}

// 获得ADC值
// ch:通道值 0~16
// 返回值:转换结果
u16 Get_Adc(u8 ch)
{
    // 设置转换序列
    ADC1->SQR3 &= 0XFFFFFFE0; // 规则序列1 通道ch
    ADC1->SQR3 |= ch;
    ADC1->CR2 |= 1 << 22; // 启动规则转换通道
    while (!(ADC1->SR & 1 << 1))
        ;            // 等待转换结束
    return ADC1->DR; // 返回adc值
}
// 获取通道ch的转换值，取times次,然后平均
// ch:通道编号
// times:获取次数
// 返回值:通道ch的times次转换结果平均值
u16 Get_Adc_Average(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;
    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc(ch);
        delay_ms(3);
    }
    return temp_val / times;
}

// 使用DMA读取指定通道一次采样值
u16 Get_Adc_DMA(u8 ch)
{
    volatile u16 value = 0;

    // 设置转换通道
    ADC1->SQR3 &= 0xFFFFFFE0;
    ADC1->SQR3 |= ch;

    // 配置DMA1 通道1
    DMA1_Channel1->CCR &= ~1;                              // 关闭DMA以便配置
    DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;             // 外设地址：ADC数据寄存器
    DMA1_Channel1->CMAR = (uint32_t)&value;                // 存储目标地址
    DMA1_Channel1->CNDTR = 1;                              // 传输1次
    DMA1_Channel1->CCR = (0 << 14)                         // MEM2MEM:0
                         | (1 << 12)                       // PL:中优先级
                         | (1 << 10)                       // MSIZE:16位
                         | (1 << 8)                        // PSIZE:16位
                         | (0 << 7)                        // MINC:不自增（单值）
                         | (0 << 6)                        // PINC:不自增
                         | (0 << 5)                        // CIRC:非循环
                         | (0 << 4)                        // DIR:从外设到内存
                         | (0 << 3) | (0 << 2) | (0 << 1); // 中断关闭

    DMA1_Channel1->CCR |= 1; // 使能DMA

    // 软件启动转换
    ADC1->CR2 |= 1 << 22;

    // 等待DMA传输完成
    while (!(DMA1->ISR & (1 << 1)))
        ;
    // 清除通道1所有标志位
    DMA1->IFCR = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);

    // 关闭DMA通道
    DMA1_Channel1->CCR &= ~1;

    return value;
}

// 使用DMA读取指定通道多次采样并返回平均值
u16 Get_Adc_DMA_Average(u8 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;
    if (times == 0)
        return 0;
    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc_DMA(ch);
        // delay_ms(3);
    }
    return (u16)(temp_val / times);
}
