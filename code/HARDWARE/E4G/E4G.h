#ifndef _E4G_H_
#define _E4G_H_
// 单片机头文件
#include "stm32f10x.h"
#define REV_OK 0    // 接收完成标志
#define REV_WAIT 1  // 接收未完成标志
#define buf_len 256 // 串口接受总长度

#define Bamfa_USART1 0
#define Bamfa_USART2 1
#define Bamfa_USART3 0

#define Bamfa_USART USART2

// 巴法云用户秘钥修改
#define BEMFA_ID "f5e57f8881b2464caf90dde017a57721" // 需要改的地方

#define DATA_TOPIC "data"

// 巴法云订阅主题指令
#define E4G_TOPIC "cmd=1&uid=" BEMFA_ID "&topic=control\r\n" // 需要改的地方

#define Return_Time "cmd=7&uid=" BEMFA_ID "&type=1\r\n" // 需要改的地方

// struct Data
// {
// 	float longitude, latitude;
// };

extern unsigned char E4G_buf[];
extern unsigned short E4G_cnt;

void E4G_Clear(void);

_Bool E4G_WaitRecive(void);

_Bool E4G_SendCmd(char *cmd, char *res);

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

void E4G_SendData(unsigned char *data);

void E4G_Init(unsigned int bound);

void USART2_IRQHandler(void);

void mode_choice(void);

void E4G_GET_GPS_Data(float *longitude, float *latitude);
#endif
