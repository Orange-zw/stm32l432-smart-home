#ifndef __GPS_H
#define __GPS_H
#include "sys.h"

#define GPS_USART1 1
#define GPS_USART2 2
#define GPS_USART3 3

// 只需改这个串口号即可↓↓↓↓↓↓↓↓↓↓↓↓
#define GPS_USART 3
#define USART_REC_LEN 200 // 定义最大接收字节数 200

// 定义数组长度
#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2
typedef struct SaveData
{
	char GPS_Buffer[GPS_Buffer_Length];
	char isGetData;					  // 是否获取到GPS数据
	char isParseData;				  // 是否解析完成
	char UTCTime[UTCTime_Length];	  // UTC时间
	char latitude[latitude_Length];	  // 纬度
	char N_S[N_S_Length];			  // N/S
	char longitude[longitude_Length]; // 经度
	char E_W[E_W_Length];			  // E/W
	char isUsefull;					  // 定位信息是否有效
} _SaveData;

extern char dingwei_flag;
// GPS函数声明
void GPS_Init(u32 bound);
void GPS_Data_Transfor(float *longitude, float *latitude);
// ...existing code...
float GPS_Calculate_Distance(float lon1, float lat1, float lon2, float lat2);
// ...existing code...
void errorLog(int num);
void parseGpsBuffer(void);
void CLR_Buf(void);
u8 Hand(char *a);
void clrStruct(void);
#endif
