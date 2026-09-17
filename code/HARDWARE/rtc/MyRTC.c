#include "stm32f10x.h" // Device header
#include "stm32f10x.h" // Device header
#include "MyRTC.h"
#include <time.h>

uint16_t Set_time[6] = {2025, 2, 11, 22, 00, 54};
uint16_t Read_time[6] = {0000, 00, 00, 00, 00, 00};

calendar_t calendar;	  // 时钟结构体
calendar_t Alarm1 = {8};  // 闹钟结构体
calendar_t Alarm2 = {12}; // 闹钟结构体
calendar_t Alarm3 = {20}; // 闹钟结构体

void MyRTC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

	PWR_BackupAccessCmd(ENABLE);

	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
		BKP_DeInit();

		RCC_LSEConfig(RCC_LSE_ON);
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
			;

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		RCC_RTCCLKCmd(ENABLE);

		RTC_WaitForSynchro();
		RTC_WaitForLastTask();

		RTC_SetPrescaler(32767);
		RTC_WaitForLastTask();

		MyRTC_SetTime(Set_time[0], Set_time[1], Set_time[2], Set_time[3], Set_time[4], Set_time[5]);

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	}
	else
	{
		RTC_WaitForSynchro();
		RTC_WaitForLastTask();
	}
}

/*
void MyRTC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

	PWR_BackupAccessCmd(ENABLE);

	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
		BKP_DeInit();

		RCC_LSICmd(ENABLE);
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		RCC_RTCCLKCmd(ENABLE);

		RTC_WaitForSynchro();
		RTC_WaitForLastTask();

		RTC_SetPrescaler(40000 - 1);
		RTC_WaitForLastTask();

		MyRTC_SetTime();

		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	}
	else
	{
		RCC_LSICmd(ENABLE);
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		RCC_RTCCLKCmd(ENABLE);

		RTC_WaitForSynchro();
		RTC_WaitForLastTask();
	}
}*/

void MyRTC_SetTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
	time_t time_cnt;
	struct tm time_date;

	time_date.tm_year = year - 1900;
	time_date.tm_mon = month - 1;
	time_date.tm_mday = day;
	time_date.tm_hour = hour;
	time_date.tm_min = minute;
	time_date.tm_sec = second;

	time_cnt = mktime(&time_date) - 8 * 60 * 60;

	RTC_SetCounter(time_cnt);
	RTC_WaitForLastTask();
}

void MyRTC_ReadTime(void)
{
	time_t time_cnt;
	struct tm time_date;

	time_cnt = RTC_GetCounter() + 8 * 60 * 60;

	time_date = *localtime(&time_cnt);

	// Read_time[0] = time_date.tm_year + 1900;
	// Read_time[1] = time_date.tm_mon + 1;
	// Read_time[2] = time_date.tm_mday;
	// Read_time[3] = time_date.tm_hour;
	// Read_time[4] = time_date.tm_min;
	// Read_time[5] = time_date.tm_sec;
	calendar.w_year = time_date.tm_year + 1900;
	calendar.w_month = time_date.tm_mon + 1;
	calendar.w_date = time_date.tm_mday;
	calendar.hour = time_date.tm_hour;
	calendar.min = time_date.tm_min;
	calendar.sec = time_date.tm_sec;
}

uint32_t MyRTC_GetTick(calendar_t *time_struct)
{
	return (time_struct->hour * 3600 + time_struct->min * 60 + time_struct->sec);
}
