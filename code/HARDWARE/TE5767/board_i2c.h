#ifndef _BOARD_I2C_H_
#define _BOARD_I2C_H_

/*********************************************************************
 * INCLUDES
 */
#include "stdint.h"

/*********************************************************************
 * DEFINITIONS
 */
// I2C_SCL时钟
#define IIC_SCL_CLK RCC_APB2Periph_GPIOB  // GPIO端口时钟
#define IIC_SCL_PORT GPIOB                // GPIO端口
#define IIC_SCL_PIN GPIO_Pin_1            // GPIO引脚
// I2C_SDA时钟
#define IIC_SDA_CLK RCC_APB2Periph_GPIOB  // GPIO端口时钟
#define IIC_SDA_PORT GPIOB                // GPIO端口
#define IIC_SDA_PIN GPIO_Pin_0            // GPIO引脚

/*********************************************************************
 * MACROS
 */
#define IIC_SCL_0() GPIO_ResetBits(IIC_SCL_PORT, IIC_SCL_PIN)
#define IIC_SCL_1() GPIO_SetBits(IIC_SCL_PORT, IIC_SCL_PIN)
#define IIC_SDA_0() GPIO_ResetBits(IIC_SDA_PORT, IIC_SDA_PIN)
#define IIC_SDA_1() GPIO_SetBits(IIC_SDA_PORT, IIC_SDA_PIN)
#define IIC_SDA_READ() GPIO_ReadInputDataBit(IIC_SDA_PORT, IIC_SDA_PIN)

/*********************************************************************
 * API FUNCTIONS
 */
void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
void IIC_SendByte(uint8_t ucByte);
uint8_t IIC_ReadByte(void);
uint8_t IIC_WaitAck(void);
void IIC_Ack(void);
void IIC_NAck(void);
uint8_t IIC_CheckDevice(uint8_t address);

#endif /* _BOARD_I2C_H_ */
