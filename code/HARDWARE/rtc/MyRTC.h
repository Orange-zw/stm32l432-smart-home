#ifndef __MYRTC_H
#define __MYRTC_H
#include "sys.h"

extern uint16_t Set_time[6];
extern uint16_t Read_time[6];

typedef struct
{
	// 公历日月年周
	int16_t w_year;
	int16_t w_month;
	int16_t w_date;
	int16_t hour;
	int16_t min;
	int16_t sec;
} calendar_t;

extern calendar_t calendar; // 日历结构体
extern calendar_t Alarm1;	// 闹钟结构体
extern calendar_t Alarm2;	// 闹钟结构体
extern calendar_t Alarm3;	// 闹钟结构体

void MyRTC_Init(void);
void MyRTC_SetTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void MyRTC_ReadTime(void);
uint32_t MyRTC_GetTick(calendar_t *time_struct);

#endif
