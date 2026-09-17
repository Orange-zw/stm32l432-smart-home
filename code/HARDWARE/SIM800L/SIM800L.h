#ifndef __SIM800L_H__
#define __SIM800L_H__
#include <stddef.h>
#include "stm32f10x.h"

/*这里修改代码即可*/
#define TEMP_HIGH_Message "5382623f6e295ea68fc79ad8002100210021"          // “厂房温度过高!!!”
#define HUMI_HIGH_Message "201c5382623f6e7f5ea68fc79ad8002100210021"      // “厂房湿度过高!!!”
#define MQ2_HIGH_Message "5382623f70df96fe6d535ea68fc79ad8002100210021"   // “厂房烟雾浓度过高!!!”
#define FIRE_DETECTED_Message "5382623f68c06d4b5230706b7130002100210021"  // “厂房检测到火焰!!!”

#define TEST_Message "5403836F65F695F45230FF0C8BF753CA65F65403836FFF01"  // 信息的Unicode码,内容为“开机测试短信”
#define Alram_Message "8BBE59075DF265AD5F008FDE63A5FF01FF01"             // 信息的Unicode码,内容为“设备已断开连接！！”

#define SIM800L_USART1 0
#define SIM800L_USART2 0
#define SIM800L_USART3 1

#define SIM800L_USART USART3

void   SIM800L_Init(unsigned int bound);
void   SIM800L_Clear(void);
_Bool  SIM800L_WaitRecive(void);
_Bool  SIM800L_SendCmd(char *cmd, char *res);
void   SIM800L_Send_Cn_message(char *number, char *content);  // 发送中文短信
void   SIM800L_Send_En_message(char *number, char *content);  // 发送英文短信
void   SIM800L_Make_Call(char *number);                       // 拨打电话
void   SIM800L_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);
size_t UTF_8_To_Unicode_String(char *str, char *utf8, size_t utf8_len);

#endif
