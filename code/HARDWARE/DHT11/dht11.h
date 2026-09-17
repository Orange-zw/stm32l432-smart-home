#ifndef __DHT11_H
#define __DHT11_H

#include "delay.h"
#include "stm32f10x.h"
#include "sys.h"

#define HIGH 1
#define LOW 0

typedef struct
{
	int32_t humi_int;  // 湿度的整数部分
	int8_t humi_deci;  // 湿度的小数部分
	int32_t temp_int;  // 温度的整数部分
	int8_t temp_deci;  // 温度的小数部分
	int8_t check_sum;  // 校验和

} DHT11_Data_t;

// DHT11设备结构体
typedef struct
{
	GPIO_TypeDef *GPIOx;      // GPIO端口 (GPIOA, GPIOB, etc.)
	uint16_t GPIO_Pin;        // GPIO引脚 (GPIO_Pin_0 ~ GPIO_Pin_15)
	uint32_t RCC_APB2Periph;  // 时钟使能 (RCC_APB2Periph_GPIOA, etc.)
	uint8_t pin_num;          // 引脚编号 (0~15)
} DHT11_Device;

// 创建DHT11设备实例
// GPIOx: GPIO端口 (GPIOA, GPIOB, GPIOC, etc.)
// GPIO_Pin: GPIO引脚 (GPIO_Pin_0 ~ GPIO_Pin_15)
// 返回: DHT11_Device指针，失败返回NULL
DHT11_Device *DHT11_Create(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

// 初始化DHT11设备
// dev: DHT11设备指针
// 返回1:不存在, 返回0:存在
u8 DHT11_Init(DHT11_Device *dev);

// 复位DHT11
// dev: DHT11设备指针
void DHT11_Rst(DHT11_Device *dev);

// 检测是否存在DHT11
// dev: DHT11设备指针
// 返回1:未检测到DHT11的存在, 返回0:存在
u8 DHT11_Check(DHT11_Device *dev);

// 从DHT11读取一个位
// dev: DHT11设备指针
// 返回值：1/0
u8 DHT11_Read_Bit(DHT11_Device *dev);

// 从DHT11读取一个字节
// dev: DHT11设备指针
// 返回值：读到的数据
u8 DHT11_Read_Byte(DHT11_Device *dev);

// 读取DHT11数据
// dev: DHT11设备指针
// DHT11_Data: 数据存储结构体指针
// 返回SUCCESS:成功, 返回ERROR:失败
uint8_t Read_DHT11(DHT11_Device *dev, DHT11_Data_t *DHT11_Data);

#endif /* __DHT11_H */
