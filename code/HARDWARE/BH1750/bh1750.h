#ifndef __BH1750_H
#define __BH1750_H
#include "sys.h"
#include "delay.h"
// IO方向设置：PC9 作为 SDA，PC8 作为 SCL
#define SDA_OUT()                    \
    {                                \
        GPIOB->CRL &= ~(0xFU << 0U); \
        GPIOB->CRL |= (0x7U << 0U);  \
    }
#define SDA_IN()                     \
    {                                \
        GPIOB->CRL &= ~(0xFU << 0U); \
        GPIOB->CRL |= (0x4U << 0U);  \
    }

// I2C 引脚映射：PC8=SCL, PC9=SDA
#define IIC_SCL PBout(1) // SCL
#define IIC_SDA PBout(0) // SDA
#define READ_SDA PBin(0) // 输入SDA
#define ADDR 0x22        // 0100011
#define uchar unsigned char

#define BHAddWrite 0x46  // 从机地址+最后写方向位
#define BHAddRead 0x47   // 从机地址+最后读方向位
#define BHPowDown 0x00   // 关闭模块
#define BHPowOn 0x01     // 打开模块等待测量指令
#define BHReset 0x07     // 重置数据寄存器值在PowerOn模式下有效
#define BHModeH1 0x10    // 高分辨率 单位1lx 测量时间120ms
#define BHModeH2 0x11    // 高分辨率模式2 单位0.5lx 测量时间120ms
#define BHModeL 0x13     // 低分辨率 单位4lx 测量时间16ms
#define BHSigModeH 0x20  // 一次高分辨率 测量 测量后模块转到 PowerDown模式
#define BHSigModeH2 0x21 // 同上类似
#define BHSigModeL 0x23  // 上类似

void Single_Write_BH1750(uchar REG_Address);
void BH1750_Init(void);
void bh_data_send(u8 command);
u16 bh_data_read(void);

// IIC所有操作函数
void BH_IIC_Init(void);                 // 初始化IIC的IO口
void BH_IIC_Start(void);                // 发送IIC开始信号
void BH_IIC_Stop(void);                 // 发送IIC停止信号
void BH_IIC_Send_Byte(u8 txd);          // IIC发送一个字节
u8 BH_IIC_Read_Byte(unsigned char ack); // IIC读取一个字节
u8 BH_IIC_Wait_Ack(void);               // IIC等待ACK信号
void BH_IIC_Ack(void);                  // IIC发送ACK信号
void BH_IIC_NAck(void);                 // IIC不发送ACK信号

void IIC_Write_One_Byte(u8 daddr, u8 addr, u8 data);
u8 IIC_Read_One_Byte(u8 daddr, u8 addr);

#endif
