#ifndef __MAX30102_H
#define __MAX30102_H
#include "sys.h"

// IO方向设置
#define SDA_IN()                  \
    {                             \
        GPIOB->CRH &= 0XFFF0FFFF; \
        GPIOB->CRH |= 8 << 16;    \
    }
#define SDA_OUT()                 \
    {                             \
        GPIOB->CRH &= 0XFFF0FFFF; \
        GPIOB->CRH |= 3 << 16;    \
    }

// IO操作函数
#define IIC_SCL PBout(13) // SCL
#define IIC_SDA PBout(12) // SDA
#define READ_SDA PBin(12) // 输入SDA

// IIC所有操作函数
void IIC_Init(void);                 // 初始化IIC的IO口
void IIC_Start(void);                // 发送IIC开始信号
void IIC_Stop(void);                 // 发送IIC停止信号
void IIC_Send_Byte(u8 txd);          // IIC发送一个字节s
u8 IIC_Read_Byte(unsigned char ack); // IIC读取一个字节
u8 IIC_Wait_Ack(void);               // IIC等待ACK信号
void IIC_Ack(void);                  // IIC发送ACK信号
void IIC_NAck(void);                 // IIC不发送ACK信号

void IIC_Write_One_Byte(u8 daddr, u8 addr, u8 data);
void IIC_Read_One_Byte(u8 daddr, u8 addr, u8 *data);

void IIC_WriteBytes(u8 WriteAddr, u8 *data, u8 dataLength);
void IIC_ReadBytes(u8 deviceAddr, u8 writeAddr, u8 *data, u8 dataLength);
#endif
