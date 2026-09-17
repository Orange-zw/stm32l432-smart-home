#ifndef __MYIIC_H
#define __MYIIC_H
#include "sys.h"

// IO方向设置
// 修改为PB6 SCL PB7 SDA
#define SDA_IN()                  \
    {                             \
        GPIOB->CRH &= 0XF0FFFFFF; \
        GPIOB->CRH |= 8 << 28;    \
    }
#define SDA_OUT()                 \
    {                             \
        GPIOB->CRH &= 0XF0FFFFFF; \
        GPIOB->CRH |= 3 << 28;    \
    }

// IO操作函数
#define IIC_SCL PBout(6) // SCL
#define IIC_SDA PBout(7) // SDA
#define READ_SDA PBin(7) // 输入SDA
                         // 输入SDA

// IIC所有操作函数
void IIC_Init(void);                 // 初始化IIC的IO口
void IIC_Start(void);                // 发送IIC开始信号
void IIC_Stop(void);                 // 发送IIC停止信号
void IIC_Send_Byte(u8 txd);          // IIC发送一个字节
u8 IIC_Read_Byte(unsigned char ack); // IIC读取一个字节
u8 IIC_Wait_Ack(void);               // IIC等待ACK信号
void IIC_Ack(void);                  // IIC发送ACK信号
void IIC_NAck(void);                 // IIC不发送ACK信号

void IIC_Write_One_Byte(u8 daddr, u8 addr, u8 data);
u8 IIC_Read_One_Byte(u8 daddr, u8 addr);

u8 iicDevReadByte(u8 devaddr, u8 addr);                  /*读一字节*/
void iicDevWriteByte(u8 devaddr, u8 addr, u8 data);      /*写一字节*/
void iicDevRead(u8 devaddr, u8 addr, u8 len, u8 *rbuf);  /*连续读取多个字节*/
void iicDevWrite(u8 devaddr, u8 addr, u8 len, u8 *wbuf); /*连续写入多个字节*/

#endif
