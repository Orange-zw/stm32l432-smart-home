#ifndef __SOFT_I2C_H__
#define __SOFT_I2C_H__

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include <stdint.h>
#include <stdbool.h>

// I2C操作结果枚举
typedef enum
{
    I2C_OK = 0,
    I2C_ERROR,
    I2C_TIMEOUT,
    I2C_NACK
} I2C_Status;

// I2C设备句柄
typedef struct
{
    GPIO_TypeDef *scl_port;
    uint16_t scl_pin;
    GPIO_TypeDef *sda_port;
    uint16_t sda_pin;
    uint32_t delay_us;
    bool initialized;
} I2C_HandleTypeDef;

// 初始化函数
I2C_Status SoftI2C_Init(I2C_HandleTypeDef *hi2c);
I2C_Status SoftI2C_Deinit(I2C_HandleTypeDef *hi2c);

// 基础操作函数
I2C_Status SoftI2C_Start(I2C_HandleTypeDef *hi2c);
I2C_Status SoftI2C_Stop(I2C_HandleTypeDef *hi2c);
I2C_Status SoftI2C_WaitAck(I2C_HandleTypeDef *hi2c);
I2C_Status SoftI2C_SendAck(I2C_HandleTypeDef *hi2c);
I2C_Status SoftI2C_SendNack(I2C_HandleTypeDef *hi2c);
I2C_Status SoftI2C_SendByte(I2C_HandleTypeDef *hi2c, uint8_t data);
uint8_t SoftI2C_ReadByte(I2C_HandleTypeDef *hi2c);

// 高级操作函数
I2C_Status SoftI2C_WriteByte(I2C_HandleTypeDef *hi2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
I2C_Status SoftI2C_ReadByte(I2C_HandleTypeDef *hi2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);
I2C_Status SoftI2C_WriteBytes(I2C_HandleTypeDef *hi2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
I2C_Status SoftI2C_ReadBytes(I2C_HandleTypeDef *hi2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
I2C_Status SoftI2C_ProbeDevice(I2C_HandleTypeDef *hi2c, uint8_t dev_addr);

// 延时函数声明（需要用户实现）
extern void delay_us(uint32_t xus);

#endif /* __SOFT_I2C_H__ */