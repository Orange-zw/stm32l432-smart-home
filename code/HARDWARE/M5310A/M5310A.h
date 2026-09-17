#ifndef _M5310A_H_
#define _M5310A_H_

#include "stm32f10x.h"

// M5310A复位引脚定义
#define M5310A_RST_RCC_CLK RCC_APB2Periph_GPIOA
#define M5310A_RST_PROT GPIOA
#define M5310A_RST_PIN GPIO_Pin_4

#define BEMFA_ID "7df9acef5bdd4f219eca0f7a092bddde"

#define DATA_TOPIC "data"
#define CONTROL_TOPIC "control"
#define ONLINE_TOPIC "event"

// 接收缓冲区大小
#define M5310A_BUF_SIZE 256

// 对外API
void M5310A_Init(USART_TypeDef *instance);
void M5310A_Clear(void);
_Bool M5310A_RxReady(void);
void M5310A_GetRxData(unsigned char *data);
void M5310A_SendData(unsigned char *uid, unsigned char *topic, unsigned char *msg);
_Bool M5310A_SendCmd(char *cmd, char *res);

// 内部使用的接收缓冲区（供外部访问，兼容旧代码）
extern unsigned char M5310A_buf[M5310A_BUF_SIZE];
extern unsigned short M5310A_cnt;

#endif
