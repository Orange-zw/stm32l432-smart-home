#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "DI.h"
#include "DO.h"
#include "delay.h"
#include "inv_mpu.h"
#include "stdbool.h"
#include "stm32f10x.h"
#include "string.h"
#include "sys.h"

/* 系统头文件 */
#include "Timer.h"
#include "esp8266.h"
#include "key.h"
#include "ssd1306.h"
#include "stmflash.h"
#include "utility.h"

/* 模块头文件 */
#include "GPS.h"
#include "algorithm.h"
#include "hc.h"
#include "max30102.h"
#include "mpu6050.h"
#include "myrtc.h"
#include "oled.h"
#include "pwm_driver.h"

/* 工具宏定义 */
#define BTN_PAGE_SWITCH 1
#define BTN_ITEM_SWITCH 2
#define BTN_UP 3
#define BTN_DOWN 4

#define CONFIG_FLAG (0x12345678)
#define EVENT_SET(event, code) memcpy(&event, &EVENT_VAL[code], sizeof(Event_t))

// 工作模式枚举
enum Mode
{
	MANUAL = 0,
	AUTO,
	MODE_MAX,
} MODE = MODE_MAX;
const char *MODE_STR[MODE_MAX] = {
    "手动",
    "自动",
};

/**
 *  数据结构体
 *
 * */
struct Data
{
	uint32_t      heart_rate;        // 心率
	uint32_t      spo2;              // 血氧
	int           led_gear;          // 灯光档位
	float         dis;               // 障碍物距离
	unsigned long steps;             // 步数
	float         yaw, pitch, roll;  // 姿态角
	uint8_t       fall_time;         // 摔倒时间
	float         lat, lon;          // 经纬度
} DATA = {0};

// 配置结构体 存放阈值 保存到Flash
struct Config
{
	const uint32_t flag;

	uint32_t   heart_rate_h;  // 心率上限
	uint32_t   spo2_l;        // 血氧下限
	float      dis_l;         // 障碍物距离下限
	calendar_t alarm_time;    // 闹钟时间
} CONFIG = {
    .flag = CONFIG_FLAG,
    .heart_rate_h = 120,
    .spo2_l = 90,
    .dis_l = 20,
    .alarm_time = {0},
};

// 事件枚举
enum EventCode
{
	EVENT_NORMAL = 0,
	EVENT_FALL,
	EVENT_HEART_RATE_HIGH,
	EVENT_SPO2_LOW,
	EVENT_DIS_LOW,
	EVENT_ALARM,
	EVENT_CODE_MAX,
};

typedef struct
{
	char           str[32];
	enum EventCode code;
	bool           isUpload;
} Event_t;
Event_t EVENT_VAL[EVENT_CODE_MAX] = {
    {"正常      ", EVENT_NORMAL, true},
    {"检测到摔倒", EVENT_FALL, true},
    {"心率过高  ", EVENT_HEART_RATE_HIGH, true},
    {"血氧过低  ", EVENT_SPO2_LOW, true},
    {"距离过近  ", EVENT_DIS_LOW, true},
    {"闹钟      ", EVENT_ALARM, false},
};

struct EventManager_t
{
	Event_t cur;   // 当前事件
	Event_t last;  // 上一个事件
} EVENT = {
    // 启动时强制触发一次事件处理
    .last = {"", EVENT_CODE_MAX, false},
};

// 系统运行时间
uint64_t RUN_TIME = 0, LAST_TIME = 0;
// OLED显示
// 按键状态
u8 key_num = 0;
// ESP8266相关
extern unsigned short esp8266_cnt;

char TIMER_IT = 0, Flagout = 1;
char data[200];

// 页面管理
void Page_data1(void);
void Page_data2(void);
void Page_data3(void);
void Page_control(void);
void Page_settings(void);
void Page_test(void);
void Page_HeartRate(void);
void Page_set_time(const char *title, calendar_t *time, bool has_date, bool has_sec);
void (*PAGES[])(void) = {
    Page_data1,
    Page_data2,
    Page_HeartRate,
    Page_control,
    Page_settings,
};
uint8_t page_count = sizeof(PAGES) / sizeof(PAGES[0]);
uint8_t page_idx = 0;
uint8_t item_idx_min = true;
uint8_t item_max_count = 4;
uint8_t items[5][128];
uint8_t items_count = 0;
uint8_t item_idx = 0;
uint8_t item_idx_start = 0;

// 设备指针
DO_handle_t  beep;
PWM_handle_t led;

/* 函数声明 */
void Switch_Mode(enum Mode new_mode);
void SAVE_config();
void LOAD_config();
void MANUAL_Handle();
void AUDIO_Handle();
void AUTO_Handle();
void APP_Handle();
void EVENT_Handle();
void SENSOR_Handle();
void KEY_Handle();
void TIMER_Handle();     // 定时器处理函数 低实时
void TIMER_IT_Handle();  // 定时器处理函数 中断调用 高实时

// 业务函数
void Beep_Beep(int duration_ms);

#define PPG_WAVE_COUNT 4
#define PPG_MIN_COUNT 0X3FFFF
uint32_t aun_ir_buffer[500];        // IR数组,红外光原始数据缓存区
uint32_t aun_red_buffer[500];       // Red数组,红光原始数据缓存区
int32_t  n_ir_buffer_length = 500;  // 数据长度

int32_t sp02;        // 血氧值
int32_t heart_rate;  // 心率值

int8_t  ch_spo2_valid;  // 算法执行成功标志位
int8_t  ch_hr_valid;
uint8_t SPO2_Ststus = Spo2_WorkMode;
u8      Packet[8];
u8      hrH = 60, spo2H = 95, mode, key_num;

uint32_t un_min, un_max;  // 原始数据的最大/最小值
uint32_t Wave_Range;      // 用于显示计算倍率
u16      i;
u8       buf_flag = 0;

s8   Wave_sum;  // OLED波形大小
u8   temp[6];   // 拼字节
u8   str[30];   // 字符串显示变量
u8   dis_hr = 0, dis_spo2 = 0, Count_hr_UpLim = 200, Count_hr_LowLim = 50, Count_spo2_UpLim = 100, Count_spo2_LowLim = 90;
u8   wave;
void OLED_Show(void);
void Finger_off(void);

const char *LED_GEAR_STR[4] = {"关闭", "低档", "中档", "高档"};
void        led_control(int state)
{
	DATA.led_gear = _constrain(DATA.led_gear + state, 0, 3);

	int led_gear_value[4] = {0, 30, 70, 100};
	PWM_OP(led, setDutyCycle, led_gear_value[DATA.led_gear]);
}

int main(void)
{
	/* 系统默认初始化 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	SystemInit();
	Key_Init();
	Timer_Init(72, 1000);  // 72MHz时钟，预分频72，周期1000 -> 1kHz -> 1ms中断
	oled_init(GPIOB, GPIO_Pin_8, GPIOB, GPIO_Pin_9);

	/* 设备初始化 */
	beep = DO_Create(GPIOC, GPIO_Pin_13, IO_HIGH);
	led = PWM_Create(GPIOA, GPIO_Pin_7);
	PWM_OP(led, setDutyCycle, 0);

	/* 模块初始化 */
	MyRTC_Init();
	GPS_Init(9600);
	max30102_init();
	MPU6050_Init();
	Hcsr04Init();

	/* 模式初始化 */
	Switch_Mode(MANUAL);
	LOAD_config();

	/* 是否选择联网 */
	do
	{
		oled_draw_text(0, 0, (uint8_t *)"是否要联网", 1);
		oled_draw_text(0, 32, (uint8_t *)"1.是  2.否", 1);
		oled_refresh_gram();
		key_num = KEY_Scan(0);
		if (key_num == 1)
		{
			oled_clear_screen(0);
			oled_refresh_gram();
			ESP8266_Init(115200);
			break;
		}
	} while (key_num != 2);
	oled_clear_screen(0);
	oled_refresh_gram();

	/* 主循环 */
	while (1)
	{
		KEY_Handle();     // 按键处理
		SENSOR_Handle();  // 传感器数据处理
		AUDIO_Handle();   // 语音命令处理
		APP_Handle();     // 云平台命令处理
		EVENT_Handle();   // 事件处理
		TIMER_Handle();   // 定时器处理

		if (MODE == MANUAL)
		{
			MANUAL_Handle();
		}
		else if (MODE == AUTO)
		{
			AUTO_Handle();
		}
		PAGES[page_idx]();
	}
	return 0;
}

void MANUAL_Handle()
{
	// 手动模式处理
}

void AUDIO_Handle()
{
}

void APP_Handle()
{
	char *ptr;

	if (esp8266_cnt > 0)
	{

		/* 参数设置 */
		if ((ptr = strstr((char *)esp8266_buf, "heart_rate_h=")) != NULL)
		{
			sscanf(ptr, "heart_rate_h=%u", &CONFIG.heart_rate_h);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "spo2_l=")) != NULL)
		{
			sscanf(ptr, "spo2_l=%u", &CONFIG.spo2_l);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "dis_l=")) != NULL)
		{
			sscanf(ptr, "dis_l=%f", &CONFIG.dis_l);
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "alarm_time=")) != NULL)
		{
			sscanf(ptr, "alarm_time=%hu:%hu", &CONFIG.alarm_time.hour, &CONFIG.alarm_time.min);
			SAVE_config();
			Beep_Beep(20);
		}

		/* 设备控制 */
		if ((ptr = strstr((char *)esp8266_buf, "led_gear=")) != NULL)
		{
			int state;
			sscanf(ptr, "led_gear=%d", &state);
			led_control(state);
			Beep_Beep(20);
		}

		/* 模式切换 */
		if ((ptr = strstr((char *)esp8266_buf, "mode=")) != NULL)
		{
			uint8_t mode;
			sscanf(ptr, "mode=%hhu", &mode);

			Switch_Mode((enum Mode)mode);
			SAVE_config();
			Beep_Beep(20);
		}

		/* 清除缓存 */
		ESP8266_Clear();
	}
}

void AUTO_Handle()
{
	// 单次只能处理一个事件，优先级从高到低
	if (DATA.fall_time >= 3)
	{
		EVENT_SET(EVENT.cur, EVENT_FALL);
	}
	else if (calendar.hour == CONFIG.alarm_time.hour &&
	         calendar.min == CONFIG.alarm_time.min &&
	         calendar.sec == 0)
	{
		EVENT_SET(EVENT.cur, EVENT_ALARM);
	}
	else if (DATA.heart_rate > CONFIG.heart_rate_h)
	{
		EVENT_SET(EVENT.cur, EVENT_HEART_RATE_HIGH);
	}
	else if (DATA.spo2 > 0 && DATA.spo2 < CONFIG.spo2_l)
	{
		EVENT_SET(EVENT.cur, EVENT_SPO2_LOW);
	}
	else if (DATA.dis < CONFIG.dis_l)
	{
		EVENT_SET(EVENT.cur, EVENT_DIS_LOW);
	}
	else
	{
		EVENT_SET(EVENT.cur, EVENT_NORMAL);
	}
}

void EVENT_Handle()
{
	if (memcmp(&EVENT.cur, &EVENT.last, sizeof(Event_t)) != 0)
	{
		Event_t last_event;
		EVENT_SET(last_event, EVENT.last.code);
		EVENT_SET(EVENT.last, EVENT.cur.code);
		if (Flagout == 0)
		{
			enum EventCode code = EVENT_NORMAL;
			if (EVENT.cur.isUpload)
			{
				code = EVENT.cur.code;
			}
			sprintf(data, "cmd=2&uid=%s&topic=event&msg=$%d$event_data$", BEMFA_ID, code);
			ESP8266_SendData((unsigned char *)data);
		}

		// 处理上一个事件的善后
		switch (last_event.code)
		{
		case EVENT_FALL:
			break;
		case EVENT_HEART_RATE_HIGH:
			break;
		case EVENT_SPO2_LOW:
			break;
		case EVENT_DIS_LOW:
			break;
		case EVENT_ALARM:
			break;
		default:
			break;
		}

		// 处理当前事件
		switch (EVENT.cur.code)
		{
		case EVENT_NORMAL:
			DO_OP(beep, close);
			break;
		case EVENT_FALL:
			DO_OP(beep, open);
			break;
		case EVENT_HEART_RATE_HIGH:
			DO_OP(beep, open);
			break;
		case EVENT_SPO2_LOW:
			DO_OP(beep, open);
			break;
		case EVENT_DIS_LOW:
			DO_OP(beep, open);
			break;
		case EVENT_ALARM:
			DO_OP(beep, open);

			oled_clear_screen(0);
			oled_draw_text_line(1, 0, (uint8_t *)"闹钟时间到！", 1);
			oled_refresh_gram();
			DO_OP(beep, open);
			int remain_time = 10 * 1000;  // 10秒
			while (KEY_Scan(0) == 0)
			{
				oled_draw_text_line_fmt(2, 0, NORMAL, " %d秒后自动关闭", remain_time / 1000);
				delay_ms(10);
				remain_time -= 10;
				if(remain_time % 1000 == 0)
				{
					oled_refresh_gram();
				}
				if (remain_time <= 0)
				{
					break;
				}
			}
			DO_OP(beep, close);
			oled_clear_screen(0);
			oled_refresh_gram();
			break;
		default:
			break;
		}
	}
}

/***
 * 界面一
 * 时间
 * 状态
 * 步数
 * 距离
 */
void Page_data1()
{
	/* 按键处理 */

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "%02d/%02d %02d:%02d:%02d", calendar.w_month, calendar.w_date, calendar.hour, calendar.min, calendar.sec);
	sprintf((char *)items[items_count++], "状态: %s", EVENT.cur.str);
	sprintf((char *)items[items_count++], "步数: %lu 步", DATA.steps);
	sprintf((char *)items[items_count++], "距离: %.1fcm", DATA.dis);

	/* 页面显示 */
	item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	item_idx_start = _max(0, item_idx - item_max_count + 1);
	for (int i = 0; i < _min(item_max_count, items_count); i++)
	{
		oled_clear_line(i);
		oled_draw_text_line(i, 0, (uint8_t *)items[item_idx_start + i], 1);
	}
	oled_refresh_gram();
	oled_clear_screen(0);
}

/**
 * 界面二
 * 经度
 * 纬度
 */
void Page_data2()
{
	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "经度: %.6f", DATA.lon);
	sprintf((char *)items[items_count++], "纬度: %.6f", DATA.lat);

	/* 页面显示 */
	item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	item_idx_start = _max(0, item_idx - item_max_count + 1);
	for (int i = 0; i < _min(item_max_count, items_count); i++)
	{
		oled_clear_line(i);
		oled_draw_text_line(i, 0, (uint8_t *)items[item_idx_start + i], 1);
	}
	oled_refresh_gram();
	oled_clear_screen(0);
}

/**
 * 控制界面
 * 模式
 * 灯光
 * 闹钟
 */
void Page_control()
{
	/* 按键处理 */
	if (key_num == BTN_UP || key_num == BTN_DOWN)
	{
		Beep_Beep(20);
		switch (item_idx)
		{
		case 0:
			Switch_Mode(key_num == BTN_UP ? AUTO : MANUAL);
			break;
		case 1:
			led_control(key_num == BTN_UP ? 1 : -1);
			break;
		case 2:
			Page_set_time("闹钟设置", &CONFIG.alarm_time, false, true);
			break;
		default:
			break;
		}
		SAVE_config();
	}

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "模式: %s", MODE_STR[MODE]);
	sprintf((char *)items[items_count++], "灯光: %s", LED_GEAR_STR[DATA.led_gear]);
	sprintf((char *)items[items_count++], "闹钟: %02d:%02d", CONFIG.alarm_time.hour, CONFIG.alarm_time.min);

	/* 页面显示 */
	item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	item_idx_start = _max(0, item_idx - item_max_count + 1);

	for (int i = 0; i < _min(item_max_count, items_count); i++)
	{
		oled_clear_line(i);
		oled_draw_text_line(i, 0, (uint8_t *)items[item_idx_start + i],
		                    item_idx_start + i == item_idx ? INVERSE : NORMAL);
	}
	oled_refresh_gram();
	oled_clear_screen(0);
}

/**
 * 设置界面

 * 距离下限
 * 心率上限
 * 血氧下限
 */
void Page_settings()
{
	/* 按键处理 */
	if (key_num == BTN_UP || key_num == BTN_DOWN)
	{
		switch (item_idx)
		{
		case 0:
			CONFIG.heart_rate_h += (key_num == BTN_UP) ? 5 : -5;
			break;
		case 1:
			CONFIG.spo2_l += (key_num == BTN_UP) ? 5 : -5;
			break;
		case 2:
			CONFIG.dis_l += (key_num == BTN_UP) ? 1.0 : -1.0;
			break;
		default:
			break;
		}
		SAVE_config();
		Beep_Beep(20);
	}

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "心率上限: %u", CONFIG.heart_rate_h);
	sprintf((char *)items[items_count++], "血氧下限: %u", CONFIG.spo2_l);
	sprintf((char *)items[items_count++], "距离下限: %.0f", CONFIG.dis_l);

	/* 页面显示 */
	item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	item_idx_start = _max(0, item_idx - item_max_count + 1);
	for (int i = 0; i < _min(item_max_count, items_count); i++)
	{
		oled_clear_line(i);
		oled_draw_text_line(i, 0, (uint8_t *)items[item_idx_start + i], item_idx_start + i == item_idx ? INVERSE : NORMAL);
	}
	oled_refresh_gram();
}

void SENSOR_Handle()
{
	// 读取传感器数据
	dmp_get_pedometer_step_count(&DATA.steps);
	mpu_dmp_get_data(&DATA.pitch, &DATA.roll, &DATA.yaw);

	parseGpsBuffer();
	GPS_Data_Transfor(&DATA.lat, &DATA.lon);

	DATA.dis = Hcsr04GetLength();
}

void TIMER_Handle()
{
	if (TIMER_IT == 0)
		return;
	TIMER_IT = 0;

	MyRTC_ReadTime();

	// 摔倒检测
	if (DATA.pitch > 25 || DATA.pitch < -25 || DATA.roll > 25 || DATA.roll < -25)
	{
		DATA.fall_time += 1;
	}
	else
	{
		DATA.fall_time = 0;
	}

	if (Flagout == 0)
	{
		// 上传DATA数据
		// #模式#距离#步数#心率#血氧#灯光档位
		sprintf(data, "cmd=2&uid=%s&topic=data&msg=", BEMFA_ID);
		_strcat_fmt(data, "#%d", MODE);
		_strcat_fmt(data, "#%.1f", DATA.dis);
		_strcat_fmt(data, "#%lu", DATA.steps);
		_strcat_fmt(data, "#%d", DATA.heart_rate);
		_strcat_fmt(data, "#%d", DATA.spo2);
		_strcat_fmt(data, "#%d", DATA.led_gear);
		_strcat_fmt(data, "#dev_data#");
		ESP8266_SendData((unsigned char *)data);

		if (DATA.lat != 0.0 || DATA.lon != 0.0)
		{
			memset(data, 0, sizeof(data));
			sprintf(data, "cmd=2&uid=%s&topic=GPS&msg=", BEMFA_ID);
			_strcat_fmt(data, "@%.6f", DATA.lon);
			_strcat_fmt(data, "@%.6f", DATA.lat);
			_strcat_fmt(data, "@GPS_data@");
			delay_ms(50);
			ESP8266_SendData((unsigned char *)data);
		}
	}
}

void TIMER_IT_Handle()
{
	RUN_TIME += 1;
	if (RUN_TIME % 1000 == 0)  // 每秒执行一次
	{
		TIMER_IT = 1;  // 设置标志位 1秒钟处理一次
		LAST_TIME = RUN_TIME;
	}
}

void Page_set_time(const char *title, calendar_t *time, bool has_date, bool has_sec)
{
	item_idx = 0;
	while (1)
	{
		// 按键处理
		key_num = KEY_Scan(0);
		if (key_num == BTN_UP || key_num == BTN_DOWN)
		{
			uint8_t item = has_date ? item_idx : item_idx + 2;
			switch (item)
			{
			case 1:
				time->w_month += (key_num == BTN_UP) ? 1 : -1;
				break;
			case 2:
				time->w_date += (key_num == BTN_UP) ? 1 : -1;
				break;
			case 3:
				time->hour += (key_num == BTN_UP) ? 1 : -1;
				break;
			case 4:
				time->min += (key_num == BTN_UP) ? 1 : -1;
				break;
			default:
				break;
			}
			Time_check(&time->w_year, &time->w_month, &time->w_date, &time->hour, &time->min, &time->sec);
			Beep_Beep(20);
		}
		if (key_num == BTN_PAGE_SWITCH)
		{
			item_idx = 0;
			break;
		}
		if (key_num == BTN_ITEM_SWITCH)
		{
			item_idx++;
		}

		// 页面项目
		item_idx_min = 1;
		items_count = 0;
		sprintf((char *)items[items_count++], "%s", title);
		if (has_date)
		{
			sprintf((char *)items[items_count++], "月: %02d", time->w_month);
			sprintf((char *)items[items_count++], "日: %02d", time->w_date);
		}
		if (has_sec)
		{
			sprintf((char *)items[items_count++], "时: %02d", time->hour);
			sprintf((char *)items[items_count++], "分: %02d", time->min);
		}

		// 页面显示
		item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
		item_idx_start = _max(0, item_idx - item_max_count + 1);
		for (int i = 0; i < _min(item_max_count, items_count); i++)
		{
			oled_clear_line(i);
			oled_draw_text_line(i, 0, (uint8_t *)items[item_idx_start + i], item_idx_start + i == item_idx ? INVERSE : NORMAL);
		}
		oled_refresh_gram();
		oled_clear_screen(0);
	}
}

void Page_HeartRate(void)
{
	char oled_str[64];

	OLED_Set_Pos(0, 0);
	sprintf(oled_str, "心率");
	OLED_ShowText(16, 0, (u8 *)oled_str, 0);

	sprintf(oled_str, "血氧");
	OLED_ShowText(64, 0, (u8 *)oled_str, 0);
	OLED_Show();
	if (mode == 0)  // 整个程序执行一次1S,所以设置模式时禁用主程序,保证按键检测灵敏,但第一次按键要长按1S左右
	{
		i = 0;  // 重置初始值
		un_max = 0;
		un_min = PPG_MIN_COUNT;

		for (i = 100; i < 500; i++)  // 数组往前移100->目的是丢弃500个数组中的前面100个数组,并更新100个新采集的值
		{
			aun_red_buffer[i - 100] = aun_red_buffer[i];
			aun_ir_buffer[i - 100] = aun_ir_buffer[i];

			if (i > 400)
			{
				if (un_min > aun_ir_buffer[i])
					un_min = aun_ir_buffer[i];  // 找到最小值
				if (un_max < aun_ir_buffer[i])
					un_max = aun_ir_buffer[i];  // 找到最大值
			}
		}

		Wave_Range = un_max - un_min;  // 计算波峰波谷的差值
		Wave_Range = Wave_Range / 14;  // 计算倍率

		for (i = 400; i < 500; i++)  // 填充100个新数据到数组
		{
			wave++;  // 减慢OLED刷新速度
			while (MAX30102_INT == 1);
			max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);                                                         // aun_ir_buffer[i]
			aun_red_buffer[i] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2];  // 采集和合并得到原始数据
			aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5];

			if (aun_ir_buffer[i] > 5000)  // 手指放开时一般都大于5000
			{
				if (aun_ir_buffer[i] > un_max)
					un_max = (aun_ir_buffer[i] + 100);  // 刷新倍率
				else if (aun_ir_buffer[i] < un_min)
				{
					un_min = aun_ir_buffer[i] - 200;
					Wave_Range = un_max - un_min;
					Wave_Range = Wave_Range / 14;
				}

				Wave_sum = (un_max - aun_ir_buffer[i]) / Wave_Range;  // 得到OLED显示的波形值
				buf_flag = 1;                                         // 标志位,执行算法用,主要目的是过滤掉手指放开时的自然光影响
			}
			else
			{
				Wave_sum = 1;
				buf_flag = 0;
			}

			if (Wave_sum > 16)
				Wave_sum = 16;  // 数值过大,纠正
			//			printf("%d, %d\r\n",aun_ir_buffer[i],aun_red_buffer[i],dis_hr);//输出脉搏波形到上位机显示
			// printf("%d, %d, %d, %d \r\n",aun_ir_buffer[i],un_max,un_min,Wave_Range);
			if (wave == PPG_WAVE_COUNT)
			{
				if (mode == 0)
					OLED_wave(Wave_sum);  // 显示波形
				wave = 0;
			}
			key_num = KEY_Scan(0);
			if (key_num == 1)
			{
				page_idx = (page_idx + 1) % page_count;
				// 退出心率血氧页面,清除显示数据
				DATA.heart_rate = 0;
				DATA.spo2 = 0;
				OLED_Clear(0);
				break;
			}
		}
		// 执行算法,去直流,滤波,计算波形幅值等
		if (buf_flag)  // 防止误判
		{
			maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &sp02, &ch_spo2_valid, &heart_rate, &ch_hr_valid);
		}
		if (buf_flag && sp02 < 100 && sp02 > 70)  // 划定显示区间, 光学会受自然光影响
		{
			dis_hr = heart_rate;
			dis_spo2 = sp02;
		}
		else
		{
			dis_hr = 00;
			dis_spo2 = 00;
		}

		if (dis_hr == 0 || dis_spo2 == 0)
		{
			if (mode == 0)
			{
				sprintf((char *)str, "            ");
				OLED_ShowString(0, 2, str, 16);
				sprintf((char *)str, "  --      -- ");
				OLED_ShowString(0, 4, str, 16);
			}
		}
		else
		{
			Packet[0] = dis_hr;
			Packet[1] = dis_spo2;
			if (mode == 0)
			{
				OLED_ShowNum_Heart(0, 2, dis_hr, 3, 32);
				OLED_ShowNum_Heart(66, 2, dis_spo2, 2, 32);
				DATA.heart_rate = dis_hr;
				DATA.spo2 = dis_spo2;
				OLED_ShowChar(106, 4, '%', 16);
			}
		}
		Finger_off();  // 手指靠近识别
		               // if (MODE == AUTO) AUTO_Handle();
		               // EVENT_Handle();
		               // TIMER_Handle();
	}
}

void OLED_Show(void)
{
	switch (mode)
	{
	case 0:
		break;

	case 1:
		OLED_ShowString(32, 0, "hrH:", 16);
		OLED_ShowNumSize(64, 0, hrH, 3, 16);  // 心率hrH: 60
		break;

	case 2:
		OLED_ShowString(32, 0, "spo2H:", 16);
		OLED_ShowNumSize(80, 0, spo2H, 3, 16);  // 血氧spo2H: 95
		break;
	}
}
void Finger_off(void)
{
	if (aun_ir_buffer[499] > 5000 && SPO2_Ststus == Proximity)
	{
		maxim_max30102_write_reg(REG_LED1_PA, 0x30);      // Choose value for ~ 7mA for LED1  LED 0C
		maxim_max30102_write_reg(REG_LED2_PA, 0x30);      // Choose value for ~ 7mA for LED2  IR 0D
		maxim_max30102_write_reg(REG_FIFO_CONFIG, 0x0f);  // sample avg = 1, fifo rollover=-1, fifo almost full = 17 08
		maxim_max30102_write_reg(REG_SPO2_CONFIG, 0x27);  // SPO2_ADC range = 4096nA, SPO2 sample rate (100 Hz), LED pulseWidth (400uS) 0A
		SPO2_Ststus = Spo2_WorkMode;
	}
	else if (aun_ir_buffer[499] < 2000 && SPO2_Ststus == Spo2_WorkMode)
	{
		maxim_max30102_write_reg(REG_LED1_PA, 0x00);      // Choose value for ~ 7mA for LED1  LED 0C
		maxim_max30102_write_reg(REG_LED2_PA, 0x19);      // Choose value for ~ 7mA for LED2  IR 0D
		maxim_max30102_write_reg(REG_SPO2_CONFIG, 0x27);  // SPO2_ADC range = 4096nA, SPO2 sample rate (100 Hz), LED pulseWidth (400uS) 0A
		maxim_max30102_write_reg(REG_FIFO_WR_PTR, 0x00);  // FIFO_WR_PTR[4:0] 04
		maxim_max30102_write_reg(REG_OVF_COUNTER, 0x00);  // OVF_COUNTER[4:0]05
		maxim_max30102_write_reg(REG_FIFO_RD_PTR, 0x00);  // FIFO_RD_PTR[4:0] 06
		maxim_max30102_write_reg(REG_FIFO_CONFIG, 0x00);  // sample avg = 1, fifo rollover=-1, fifo almost full = 17 08
		SPO2_Ststus = Proximity;
	}
}

void KEY_Handle()
{
	key_num = KEY_Scan(0);
	if (key_num == BTN_PAGE_SWITCH)  // 切换页面
	{
		page_idx = (page_idx + 1) % page_count;
		item_idx = 0;

		// 清屏
		oled_clear_screen(0);
		oled_refresh_gram();
	}
	if (key_num == BTN_ITEM_SWITCH)
	{
		item_idx++;
	}
}
void SAVE_config()
{
	CONFIG.heart_rate_h = _constrain(CONFIG.heart_rate_h, 0, 200);
	CONFIG.spo2_l = _constrain(CONFIG.spo2_l, 0, 100);
	CONFIG.dis_l = _constrain(CONFIG.dis_l, 3, 100);

	STMFLASH_WriteBuf(FLASH_SAVE_ADDR, &CONFIG, sizeof(CONFIG));
}

void LOAD_config()
{
	struct Config temp_config;
	STMFLASH_ReadBuf(FLASH_SAVE_ADDR, &temp_config, sizeof(temp_config));
	if (temp_config.flag == CONFIG_FLAG)  // 如果标志位正确，则加载配置
	{
		memcpy(&CONFIG, &temp_config, sizeof(CONFIG));
	}
	else  // 如果标志位不正确，则初始化配置
	{
		SAVE_config();
	}
}

void Switch_Mode(enum Mode new_mode)
{
	if (MODE == new_mode || new_mode >= MODE_MAX)
	{
		return;
	}
	EVENT_SET(EVENT.cur, EVENT_NORMAL);
	MODE = new_mode;

	// 切换到模式时关闭所有设备
	DO_OP(beep, close);
	// PWM_OP(led, setDutyCycle, 0);
	// DATA.led_gear = 0;
}

void Beep_Beep(int duration_ms)
{
	if (DO_OP(beep, is_open))
		return;
	DO_OP(beep, open);
	delay_ms(duration_ms);
	DO_OP(beep, close);
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIMER_IT_Handle();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

// 禁用printf
int fputc(int ch, FILE *f)
{
	// 什么都不做,直接返回
	return ch;
}