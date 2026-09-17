#include "utility.h"

// 获取某年某月的天数（考虑闰年）
static int Days_in_month(int year, int month)
{
	static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int              d = days[month - 1];
	if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
		d = 29;
	return d;
}

// 时间日期校准：60进制进位 + 月份校验（含闰年）
void Time_check(int16_t* year, int16_t* month, int16_t* day, int16_t* hour, int16_t* min, int16_t* sec)
{
	int carry;

	/* 秒 -> 分：60进制进位 */
	carry = *sec / 60;
	*sec %= 60;
	if (*sec < 0)
	{
		*sec += 60;
		carry--;
	}
	*min += carry;

	/* 分 -> 时：60进制进位 */
	carry = *min / 60;
	*min %= 60;
	if (*min < 0)
	{
		*min += 60;
		carry--;
	}
	*hour += carry;

	/* 时 -> 日：24进制进位 */
	carry = *hour / 24;
	*hour %= 24;
	if (*hour < 0)
	{
		*hour += 24;
		carry--;
	}
	*day += carry;

	/* 月份、年份范围保护 */
	if (*month < 1)
		*month = 1;
	if (*month > 12)
		*month = 12;
	if (*year < 1)
		*year = 1;

	/* 日期按当月天数进位，并做月份/年份进位 */
	while (*day > Days_in_month(*year, *month))
	{
		*day -= Days_in_month(*year, *month);
		(*month)++;
		if (*month > 12)
		{
			*month = 1;
			(*year)++;
		}
	}
	while (*day < 1)
	{
		(*month)--;
		if (*month < 1)
		{
			*month = 12;
			(*year)--;
			if (*year < 1)
				*year = 1;
		}
		*day += Days_in_month(*year, *month);
	}
}

uint32_t Time_sec(int16_t hour, int16_t min, int16_t sec)
{
	return hour * 3600 + min * 60 + sec;
}