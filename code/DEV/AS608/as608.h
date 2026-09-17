#ifndef __AS608_H
#define __AS608_H
#include <stdio.h>
#include "delay.h"
#include "stdint.h"
#include "usart_driver.h"

#define CharBuffer1 0x01
#define CharBuffer2 0x02

extern uint32_t AS608Addr;  // 模块地址

#define PACKET_HEAD 0xEF01
#define REG_ADDR 0xFFFFFFFF
// #define delay_ms(ms) (Sleep(ms))

#if defined(__GNUC__) && !defined(__clang__)
#define __rev16(x) _byteswap_ushort(x)
#define __rev32(x) _byteswap_ulong(x)
// CMSIS compiler
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define __rev16(x) __REV16(x)
#define __rev32(x) __REV(x)
#endif

// #pragma anon_unions
#pragma pack(push, 1)
typedef struct
{
	uint16_t head;
	uint32_t addr;
	uint8_t flag;
	uint16_t length;
	uint8_t cmd;
} PacketHeader;
#pragma pack(pop)

typedef struct
{
	PacketHeader header;
	uint8_t data[32];
	uint16_t checksum;
} Packet;

typedef enum
{
	CMD_FLAG = 0x01,
	DATA_FLAG = 0x02,
	ACK_FLAG = 0x07,
} PacketFlag;

typedef struct
{
	uint16_t pageID;     // 指纹ID
	uint16_t mathscore;  // 匹配得分
} SearchResult;

typedef struct
{
	uint16_t PS_max;   // 指纹最大容量
	uint8_t PS_level;  // 安全等级
	uint32_t PS_addr;
	uint8_t PS_size;  // 通讯数据包大小
	uint8_t PS_N;     // 波特率基数N
} SysPara;

uint8_t PS_GetImage(void);  // 录入图像

uint8_t PS_GenChar(uint8_t BufferID);  // 生成特征

uint8_t PS_Match(void);  // 精确比对两枚指纹特征

uint8_t PS_Search(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p);  // 搜索指纹

uint8_t PS_RegModel(void);  // 合并特征（生成模板）

uint8_t PS_StoreChar(uint8_t BufferID, uint16_t PageID);  // 储存模板

uint8_t PS_WriteReg(uint8_t RegNum, uint8_t DATA);  // 写系统寄存器

uint8_t PS_ReadSysPara(SysPara *p);  // 读系统基本参数

uint8_t PS_SetAddr(uint32_t addr);  // 设置模块地址

uint8_t PS_WriteNotepad(uint8_t NotePageNum, uint8_t *content);  // 写记事本

uint8_t PS_ReadNotepad(uint8_t NotePageNum, uint8_t *note);  // 读记事

uint8_t PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, uint8_t *finger_number);  // 高速搜索

uint8_t PS_ValidTempleteNum(uint8_t *ValidN);  // 读有效模板个数

void PS_Init(USART_TypeDef *instance);                           // 初始化AS608模块
void PS_Touch_Init(GPIO_TypeDef *gpio_port, uint16_t gpio_pin);  // 初始化触摸传感器
bool PS_IsTouch(void);                                           // 检测是否触摸
// void PS_setTouchCallback(void (*callback)(int id, void *args), void *args);  // 设置触摸回调函数
uint8_t PS_HandShake(void);                               // 与AS608模块握手
uint16_t PS_AutoIdentify(void);                           // 自动验证指纹
uint8_t PS_Sleep(void);                                   // 6.4.6 休眠指令
uint8_t PS_Cancel(void);                                  // 6.4.5 取消指令
void PS_AutoEnroll(uint16_t PageID, uint8_t input_time);  // 自动注册指纹  按N次注册
uint8_t PS_GetEnrollCount();                              // 获取注册进度
uint8_t PS_DeleteChar(uint16_t PageID, uint16_t N);       // 删除模板
uint8_t PS_Empty(void);                                   // 清空指纹库
const char *EnsureMessage(uint8_t ensure);                // 确认码错误信息解析
#endif
