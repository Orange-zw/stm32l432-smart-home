#ifndef __RC522_H__
#define __RC522_H__

/**
 * 模块介绍，工作于13.56MHz（高频），支持ISO/IEC 14443A协议。
 * 工作流程：复位应答(request) -> 防冲突(Anti collision Loop)，这一步就会返回卡4位ID
 *       -> 选择卡片(Select Tag) -> 三次相互验证(Authentication) -> 读写数据(Read/Write) -> 停止工作(Stop)
 * 每次改变扇区都需要进行三次相互验证
 */

#include "stm32f10x.h"

// 密码a和b
extern u8 KEY_A[6];
extern u8 KEY_B[6];

// 置零用
extern unsigned char DATA0[16];
extern unsigned char DATA1[16];
extern unsigned char DATA2[16];

void rc522Init(void);                                          // 初始化
u8 rc522IsConnected(void);                                     // 判断RC522是否连接
char pcdRequest(u8 req_code, u8 *pTagType);                    // 请求卡片
char pcdAnticoll(u8 *pSnr);                                    // 防冲撞
char pcdSelectTag(u8 *pSnr);                                   // 选卡
char pcdAuthPasswd(u8 auth_mode, u8 addr, u8 *pKey, u8 *pSnr); // 验证密码
char pcdReadData(u8 addr, u8 *p);                              // 读块数据
char pcdWriteData(u8 addr, u8 *p);                             // 写块数据

#if 0 // 以下函数未使用，待再看
    char pcdHalt(void); // 卡片进入休眠模式，没有试过
    u8 pcdGetSize(u8* serNum); // 获取卡片存储容量大小，待再看目前不准或者未找到使用方法
#endif

#endif
