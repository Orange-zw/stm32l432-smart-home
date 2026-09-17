#ifndef __DS1302_H
#define __DS1302_H
#include "sys.h"

//-------------------------------------------------------------------------*
// 文件名:  DS1302.h (实时时钟头文件)                                          *
//-------------------------------------------------------------------------*
// IO方向设置
#define DS1302_IO_IN()            \
	{                             \
		GPIOC->CRH &= 0xF0FFFFFF; \
		GPIOC->CRH |= 0x08000000; \
	}  // 低八位引脚的PC11脚定义为输入
#define DS1302_IO_OUT()           \
	{                             \
		GPIOC->CRH &= 0xF0FFFFFF; \
		GPIOC->CRH |= 0x03000000; \
	}  // 低八位引脚的PC11脚定义为输出
// IO操作函数
#define DIO_OUT PCout(14)  // 数据端口	PC11
#define DIO_IN PCin(14)    // 数据端口	PC11
#define CE PCout(13)       // PC13
#define DIO PCout(14)      // PC14
#define SCLK PCout(15)     // PC15

typedef struct
{
	u8  sec;
	u8  min;
	u8  hour;
	u8  day;
	u8  mon;
	u16 year;
	u8  week;
} _next_obj;
extern _next_obj next;
typedef struct
{
	// 公历日月年周
	s16 w_year;
	s16 w_month;
	s16 w_date;
	s16 hour;
	s16 min;
	s16 sec;
} _calendar_obj;

// extern _calendar_obj calendar;         // 日历结构体
extern u32 RTC_sec_sum;      // 当前时间的总秒值
extern u32 Program_sec_sum;  // 当前编程任务的总秒值,与RTC_sec_sum进行比较

void     RTC_Get(void);
void     NEXT_Date(u8 day);
void     IO_Init(void);
void     TIME(void);                                                                    // 显示时间
u8       RTC_Pro_count(u16 syear, u8 smon, u8 sday, u8 hour, u8 min, u8 sec, u8 mode);  // 编程任务时间计算
u8       Pro_Get_time(u32 ttt);                                                         // 编程模式无效时间时计算下次开始的日期
void     DS1302_Init(void);
void     Time_Display(void);
void     RTC_Set(u16 year, u8 mon, u8 day, u8 hour, u8 min, u8 sec);
uint32_t RTC_GetTick(_calendar_obj *time_struct);
#endif
