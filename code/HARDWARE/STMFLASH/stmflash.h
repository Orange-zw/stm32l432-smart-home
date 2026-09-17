#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "sys.h"

extern u16 A_Parameter[10], B_Parameter[10], Flash_Parameter[10]; // Flash相关数组
// 用户根据自己的需要设置
#define STM32_FLASH_SIZE 64 // 所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 1  // 使能FLASH写入(0，不是能;1，使能)

// FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 // STM32 FLASH的起始地址
#define FLASH_SAVE_ADDR 0X0800FC00  // 设置FLASH 保存地址(必须为偶数，且其值要大于本代码所占用FLASH的大小+0X08000000)0X0800E000
#define FLASH_SAVE_ADDRD 0X0800EC00

// FLASH解锁键值
#define FLASH_KEY1 0X45670123
#define FLASH_KEY2 0XCDEF89AB

void STMFLASH_Unlock(void);                                          // FLASH解锁
void STMFLASH_Lock(void);                                            // FLASH上锁
u8 STMFLASH_GetStatus(void);                                         // 获得状态
u8 STMFLASH_WaitDone(u16 time);                                      // 等待操作结束
u8 STMFLASH_ErasePage(u32 paddr);                                    // 擦除页
u8 STMFLASH_WriteHalfWord(u32 faddr, u16 dat);                       // 写入半字
u16 STMFLASH_ReadHalfWord(u32 faddr);                                // 读出半字
void STMFLASH_WriteLenByte(u32 WriteAddr, u32 DataToWrite, u16 Len); // 指定地址开始写入指定长度的数据
u32 STMFLASH_ReadLenByte(u32 ReadAddr, u16 Len);                     // 指定地址开始读取指定长度数据
void STMFLASH_Write(u32 WriteAddr, u16 *pBuffer, u16 NumToWrite);    // 从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr, u16 *pBuffer, u16 NumToRead);       // 从指定地址开始读出指定长度的数据

// float类型数据的Flash读写函数
void STMFLASH_WriteFloat(u32 WriteAddr, float *pBuffer, u16 NumToWrite); // 写入float类型数据
void STMFLASH_ReadFloat(u32 ReadAddr, float *pBuffer, u16 NumToRead);    // 读取float类型数据
void STMFLASH_ReadInt32(u32 ReadAddr, int32_t *pBuffer, u16 NumToRead);
void STMFLASH_WriteInt32(u32 WriteAddr, int32_t *pBuffer, u16 NumToWrite);
void STMFLASH_WriteBuf(u32 WriteAddr, void *pBuffer, u16 structSize); // 写入任意类型数据
void STMFLASH_ReadBuf(u32 ReadAddr, void *pBuffer, u16 structSize);   // 读取任意类型数据

void Test_Write(u32 WriteAddr, u16 WriteData); // 测试写入
void Flash_Read(void);                         // 读取Flash数据
void Flash_Write(void);                        // 写入Flash数据

#endif
