#ifndef __ESP8266_H__
#define __ESP8266_H__	 
#include "sys.h"
#include "usart.h"		
#include "delay.h"	
#include "led.h"
#include "string.h"  
#include "os.h"  

void esp8266_init(void);

u8* esp8266_check_cmd(u8 *str);
u8 esp8266_apsta_check(void);
u8 esp8266_send_data(u8 *data,u8 *ack,u16 waittime);
u8 esp8266_send_cmd(u8 *cmd,u8 *ack,u16 waittime);
u8 esp8266_quit_trans(void);
u8 esp8266_consta_check(void);
u8 Check_Connections(void);
void esp_8266_get_wanip(u8* ipbuf);
u8 Wait_Reply(u8 *ack,u16 waittime);
u8 Send_NetworkData(u8 ID,u8* data);
void Cmd_Scan(void);

extern const u8* esp_ESP8266_ECN_TBL[5];
extern u8 ESP8266_STA;
extern u8 ESP8266_Cmd[50];
extern u8 Manual_STA,AUTO_STA;
#endif





