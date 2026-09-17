# HX1838 红外遥控模块使用说明

## 概述
这是一个经过优化的HX1838红外遥控模块驱动，具有更好的可移植性和配置灵活性。

### ? 新特性
- **自动标志位管理**：`HX1838_ReadKeyValue()` 函数会自动清除接收标志位，无需手动管理
- **简化调用**：main函数中只需要调用一个函数就能获取按键值
- **智能判断**：通过 `HX1838_KEY_INVALID` 常量判断是否接收到有效按键

## 引脚配置
在 `HX1838.h` 文件的配置区域中，您可以轻松修改以下配置：

### 基本引脚配置
```c
#define HX1838_GPIO_PORT           GPIOA                    // 红外接收器连接的GPIO端口
#define HX1838_GPIO_PIN            GPIO_Pin_1               // 红外接收器连接的GPIO引脚
#define HX1838_GPIO_PIN_SOURCE     GPIO_PinSource1          // GPIO引脚源
```

### 中断配置
```c
#define HX1838_EXTI_LINE           EXTI_Line1               // 外部中断线
#define HX1838_EXTI_IRQn           EXTI1_IRQn              // 外部中断IRQ通道
#define HX1838_IRQ_HANDLER         EXTI1_IRQHandler         // 中断处理函数名
```

### 时钟配置
```c
#define HX1838_GPIO_CLK            RCC_APB2Periph_GPIOA     // GPIO时钟
#define HX1838_AFIO_CLK            RCC_APB2Periph_AFIO      // AFIO时钟
```

### 中断优先级配置
```c
#define HX1838_IRQ_PREEMPT_PRIORITY    0                    // 抢占优先级
#define HX1838_IRQ_SUB_PRIORITY        1                    // 响应优先级
```

## 使用方法

### 1. 修改引脚配置
如果您想将红外接收器连接到不同的引脚，只需要修改头文件中的相应宏定义即可。

**例如：连接到PA2引脚**
```c
#define HX1838_GPIO_PORT           GPIOA
#define HX1838_GPIO_PIN            GPIO_Pin_2
#define HX1838_GPIO_PIN_SOURCE     GPIO_PinSource2
#define HX1838_EXTI_LINE           EXTI_Line2
#define HX1838_EXTI_IRQn           EXTI2_IRQn
#define HX1838_IRQ_HANDLER         EXTI2_IRQHandler
```

**例如：连接到PB0引脚**
```c
#define HX1838_GPIO_PORT           GPIOB
#define HX1838_GPIO_PIN            GPIO_Pin_0
#define HX1838_GPIO_PIN_SOURCE     GPIO_PinSource0
#define HX1838_EXTI_LINE           EXTI_Line0
#define HX1838_EXTI_IRQn           EXTI0_IRQn
#define HX1838_IRQ_HANDLER         EXTI0_IRQHandler
```

### 2. 在main.c中使用（简化版）
```c
#include "HX1838.h"

int main(void)
{
    HX1838_Init();  // 初始化红外模块
    
    while(1)
    {
        // 直接调用函数获取按键值，无需手动管理标志位
        u8 key = HX1838_ReadKeyValue();
        
        // 如果接收到有效按键（key != HX1838_KEY_INVALID）
        if (key != HX1838_KEY_INVALID)
        {
            // 处理按键逻辑
            switch(key)
            {
                case 1: // 按键1
                    // 处理逻辑
                    break;
                case 2: // 按键2
                    // 处理逻辑
                    break;
                // ... 其他按键
            }
        }
    }
}
```

## 注意事项

1. **引脚选择限制**：STM32F103C8T6的外部中断线有限制，每个端口只能使用特定的引脚作为外部中断源。

2. **中断函数名**：不同的引脚对应不同的中断处理函数，请确保宏定义正确。

3. **时钟配置**：如果使用不同的GPIO端口，需要相应修改时钟配置。

4. **优先级设置**：根据您的系统需求调整中断优先级。

## 支持的按键
当前驱动支持以下按键：
- 数字键：0-9
- 功能键：*、#、OK
- 方向键：上、下、左、右

## 移植到其他STM32芯片
对于其他STM32芯片，只需要修改：
1. 时钟配置宏定义
2. GPIO端口和引脚定义
3. 中断线定义
4. 中断处理函数名

这样优化后的代码具有更好的可维护性和可移植性！
