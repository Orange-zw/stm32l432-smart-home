#ifndef _ESP8266_H_
#define _ESP8266_H_
// 单片机头文件
#include "stm32f10x.h"
#define REV_OK 0     // 接收完成标志
#define REV_WAIT 1   // 接收未完成标志
#define buf_len 256  // 串口接受总长度

#define Bamfa_USART1 0
#define Bamfa_USART2 1
#define Bamfa_USART3 0

#define Bamfa_USART USART2

// ESP-01S复位引脚定义，需要改的地方
#define ESP01S_RST_RCC_CLK RCC_APB2Periph_GPIOA
#define ESP01S_RST_PROT GPIOA
#define ESP01S_RST_PIN GPIO_Pin_4

// WiFi名称密码修改
#define ESP8266_WIFI_INFO "AT+CWJAP=\"qidian\",\"qdcf8888\"\r\n"  // 需要改的地方

// 巴法云网络端口（不用改）
#define ESP8266_ONENET_INFO "AT+CIPSTART=\"TCP\",\"bemfa.com\",8344\r\n"

// 巴法云用户秘钥修改
#define BEMFA_ID "d47d5080dda44b5f99e053bd006a2392"  // 需要改的地方

#define DATA_TOPIC "data"

// 巴法云订阅主题指令
#define ESP8266_TOPIC "cmd=1&uid=" BEMFA_ID "&topic=control\r\n"  // 需要改的地方

#define Return_Time "cmd=7&uid=" BEMFA_ID "&type=1\r\n"  // 需要改的地方

void ESP8266_Clear(void);

_Bool ESP8266_WaitRecive(void);

_Bool ESP8266_SendCmd(char *cmd, char *res);

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

void ESP8266_SendData(unsigned char *data);

void ESP8266_Init(unsigned int bound);

void USART2_IRQHandler(void);

void mode_choice(void);

extern unsigned char  esp8266_buf[];
extern unsigned short esp8266_cnt;

#endif
