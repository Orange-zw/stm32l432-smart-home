#ifndef _GPRS_H_
#define _GPRS_H_





#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志


void GPRS_Init(void);

void ESP8266_Clear(void);

void ESP8266_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP8266_GetIPD(unsigned short timeOut);


#endif
