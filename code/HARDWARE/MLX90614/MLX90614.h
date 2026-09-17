#ifndef __MLX90614_H
#define __MLX90614_H
#include "stm32f10x.h"
#define ACK 0
#define NACK 1
#define SA 0x00
#define TA 0x06
#define RAM_ACCESS 0x00
#define EEPROM_ACCESS 0x20
#define RAM_TOBJ1 0x07

#define SMBUS_PORT GPIOB
#define RCC_APB2Periph_SMBUS_PORT RCC_APB2Periph_GPIOB
#define SMBUS_SCK GPIO_Pin_0
#define SMBUS_SDA GPIO_Pin_1

#define SMBUS_SCK_H() SMBUS_PORT->BSRR = SMBUS_SCK
#define SMBUS_SCK_L() SMBUS_PORT->BRR = SMBUS_SCK
#define SMBUS_SDA_H() SMBUS_PORT->BSRR = SMBUS_SDA
#define SMBUS_SDA_L() SMBUS_PORT->BRR = SMBUS_SDA

#define SMBUS_SDA_PIN() SMBUS_PORT->IDR &SMBUS_SDA

void SMBus_StartBit(void);
void SMBus_StopBit(void);
void SMBus_SendBit(u8);
u8 SMBus_SendByte(u8);
u8 SMBus_ReceiveBit(void);
u8 SMBus_ReceiveByte(u8);
void SMBus_Delay(u16);
void SMBus_Init(void);
u16 SMBus_ReadMemory(u8, u8);
u8 PEC_Calculation(u8 *);
float SMBus_ReadTemp(void); // 获取温度值
float tobody(float tf);     // 温度矫正
#endif
