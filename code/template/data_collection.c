#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "AI.h"
#include "DI.h"
#include "delay.h"
#include "stdbool.h"
#include "stm32f10x.h"
#include "string.h"
#include "sys.h"

/* 系统头文件 */
#include "Timer.h"
#include "key.h"
#include "ssd1306.h"
#include "stmflash.h"
#include "utility.h"

/* 模块头文件 */
#include "DS18B20.h"
#include "ESP8266.h"
#include "GPS.h"
#include "YFS401.h"

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
	float ph;         // 酸碱度
	float flow;       // 流量
	float tds;        // 总溶解固体
	float turbidity;  // 浊度
	float temp;       // 温度

	float lat, lon;  // 经纬度
} DATA = {
    .lat = 0,
    .lon = 0,
};

// 配置结构体 存放阈值 保存到Flash
struct Config
{
	const uint32_t flag;

	float ph_h;         // 酸碱度上限
	float tds_h;        // 总溶解固体上限
	float temp_h;       // 温度上限
	float flow_h;       // 流量上限
	float turbidity_h;  // 浊度上限

} CONFIG = {
    .flag = CONFIG_FLAG,
    .ph_h = 14,
    .tds_h = 1000,
    .temp_h = 40,
    .flow_h = 100,
    .turbidity_h = 100,
};

// 事件枚举
enum EventCode
{
	EVENT_NORMAL = 0,
	EVENT_PH_HIGH,
	EVENT_TDS_HIGH,
	EVENT_TEMP_HIGH,
	EVENT_FLOW_HIGH,
	EVENT_TURBIDITY_HIGH,
	EVENT_CODE_MAX,
};

typedef struct
{
	char           str[64];
	enum EventCode code;
	bool           isUpload;
} Event_t;
Event_t EVENT_VAL[EVENT_CODE_MAX] = {
    {"正常    ", EVENT_NORMAL, true},
    {"PH过高  ", EVENT_PH_HIGH, true},
    {"TDS过高 ", EVENT_TDS_HIGH, true},
    {"温度过高", EVENT_TEMP_HIGH, true},
    {"流量过高", EVENT_FLOW_HIGH, true},
    {"浊度过高", EVENT_TURBIDITY_HIGH, true},
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
char oled_str[100];
// 按键状态
u8 key_num = 0;
// ESP8266相关
extern unsigned short esp8266_cnt;

char TIMER_IT = 0, Flagout = 1;
char data[200];
// 音频相关
uint8_t audio_cmd[128];

// 页面管理
void Page_data1(void);
void Page_data2(void);
void Page_data3(void);
void Page_control(void);
void Page_settings(void);
void Page_test(void);
void Page_HeartRate(void);
void (*PAGES[])(void) = {
    Page_data1,
    Page_data2,
    Page_control,
};
uint8_t page_count = sizeof(PAGES) / sizeof(PAGES[0]);
uint8_t page_idx = 0;
uint8_t item_idx_min = true;
uint8_t item_max_count = 4;
uint8_t items[10][256];
uint8_t items_count = 0;
uint8_t item_idx = 0;
uint8_t item_idx_start = 0;

// 设备指针
DO_handle_t beep;
AI_handle_t ph_sensor;
AI_handle_t tds_sensor;
AI_handle_t turbidity_sensor;

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

float PH_Check(void)  // pH传感器
{
	float PH_DAT;
	float PH_AD;
	PH_AD = AI_OP(ph_sensor, get_raw_value);
	PH_DAT = (PH_AD / 4096.0) * 3.3;
	PH_DAT = -5.7541 * PH_DAT + 14.54 - 0.45;
	PH_DAT = _constrain(PH_DAT, 0, 14.0);
	return PH_DAT;
}
float TS_GetData(void)  // 浊度传感器
{
	float TS_DAT;
	float TS_K = 2047.19;

	TS_DAT = AI_OP(turbidity_sensor, get_voltage, 3.3);
	TS_DAT = -865.68 * TS_DAT + TS_K;

	if (TS_DAT < 320)
	{
		TS_DAT = 0;
	}

	return TS_DAT;
}

float TDS_Check(void)  // TDS传感器
{
	float tempData = 0;
	float TDS_DAT;

	TDS_DAT = AI_OP(tds_sensor, get_voltage, 3.3);
	TDS_DAT = 66.71 * TDS_DAT * TDS_DAT * TDS_DAT - 127.93 * TDS_DAT * TDS_DAT + 428.7 * TDS_DAT;
	if (TDS_DAT < 20)
	{
		TDS_DAT = 0;
	}

	return TDS_DAT;
}

int main(void)
{
	/* 系统默认初始化 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	SystemInit();
	Key_Init();
	Timer_Init(72, 1000);  // 72MHz时钟，预分频72，周期1000 -> 1kHz -> 1ms中断
	oled_init();

	/* 设备初始化 */
	beep = DO_Create(GPIOC, GPIO_Pin_13, IO_HIGH);
	ph_sensor = AI_Create(GPIOA, GPIO_Pin_5, ADC1);
	tds_sensor = AI_Create(GPIOA, GPIO_Pin_0, ADC1);
	turbidity_sensor = AI_Create(GPIOA, GPIO_Pin_1, ADC1);

	/* 模块初始化 */
	YFS401_Init();
	DS18B20_Init();
	GPS_Init(9600);

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
		if ((ptr = strstr((char *)esp8266_buf, "ph_h=")) != NULL)
		{
			sscanf(ptr, "ph_h=%f", &CONFIG.ph_h);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "tds_h=")) != NULL)
		{
			sscanf(ptr, "tds_h=%f", &CONFIG.tds_h);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "temp_h=")) != NULL)
		{
			sscanf(ptr, "temp_h=%f", &CONFIG.temp_h);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "turbidity_h=")) != NULL)
		{
			sscanf(ptr, "turbidity_h=%f", &CONFIG.turbidity_h);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "flow_h=")) != NULL)
		{
			sscanf(ptr, "flow_h=%f", &CONFIG.flow_h);
			SAVE_config();
			Beep_Beep(20);
		}

		/* 设备控制 */

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

	if (DATA.ph > CONFIG.ph_h)
	{
		EVENT_SET(EVENT.cur, EVENT_PH_HIGH);
	}
	else if (DATA.tds > CONFIG.tds_h)
	{
		EVENT_SET(EVENT.cur, EVENT_TDS_HIGH);
	}
	else if (DATA.temp > CONFIG.temp_h)
	{
		EVENT_SET(EVENT.cur, EVENT_TEMP_HIGH);
	}
	else if (DATA.turbidity > CONFIG.turbidity_h)
	{
		EVENT_SET(EVENT.cur, EVENT_TURBIDITY_HIGH);
	}
	else if (DATA.flow > CONFIG.flow_h)
	{
		EVENT_SET(EVENT.cur, EVENT_FLOW_HIGH);
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

		// 处理当前事件
		switch (EVENT.cur.code)
		{
		case EVENT_NORMAL:
			DO_OP(beep, close);
			break;
		case EVENT_PH_HIGH:
			DO_OP(beep, open);
		case EVENT_TDS_HIGH:
			DO_OP(beep, open);
			break;
		case EVENT_TEMP_HIGH:
			DO_OP(beep, open);
			break;
		case EVENT_TURBIDITY_HIGH:
			DO_OP(beep, open);
			break;
		case EVENT_FLOW_HIGH:
			DO_OP(beep, open);
			break;
		default:
			break;
		}
	}
}

/***
 * 界面一
 * 状态
 * 水温
 * PH
 * TDS
 */
void Page_data1()
{
	/* 按键处理 */

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "状态: %s", EVENT.cur.str);
	sprintf((char *)items[items_count++], "水温: %.1f℃", DATA.temp);
	sprintf((char *)items[items_count++], "PH: %.1f", DATA.ph);
	sprintf((char *)items[items_count++], "TDS: %.0f", DATA.tds);

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

/***
 * 界面二
 * 流量
 * 浊度
 * 经度
 * 纬度
 */
void Page_data2()
{
	/* 按键处理 */

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "流量: %.0fmL/min", DATA.flow);
	sprintf((char *)items[items_count++], "浊度: %.0f", DATA.turbidity);
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
 * PH上限
 * TDS上限
 * 水温上限
 * 流量上限
 * 浊度上限
 */
void Page_control()
{
	/* 按键处理 */
	if (key_num == BTN_UP || key_num == BTN_DOWN)
	{
		switch (item_idx)
		{
		case 0:
			Switch_Mode(key_num == BTN_UP ? AUTO : MANUAL);
			break;
		case 1:
			CONFIG.ph_h += (key_num == BTN_UP) ? 1 : -1;
			break;
		case 2:
			CONFIG.tds_h += (key_num == BTN_UP) ? 1 : -1;
			break;
		case 3:
			CONFIG.temp_h += (key_num == BTN_UP) ? 1 : -1;
			break;
		case 4:
			CONFIG.flow_h += (key_num == BTN_UP) ? 1 : -1;
			break;
		case 5:
			CONFIG.turbidity_h += (key_num == BTN_UP) ? 1 : -1;
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
	sprintf((char *)items[items_count++], "模式: %s", MODE_STR[MODE]);
	sprintf((char *)items[items_count++], "PH上限: %.1f", CONFIG.ph_h);
	sprintf((char *)items[items_count++], "TDS上限: %.0f", CONFIG.tds_h);
	sprintf((char *)items[items_count++], "水温上限: %.1f℃", CONFIG.temp_h);
	sprintf((char *)items[items_count++], "流量上限: %.0fL/min", CONFIG.flow_h);
	sprintf((char *)items[items_count++], "浊度上限: %.0f", CONFIG.turbidity_h);

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

void SENSOR_Handle()
{
	// 读取传感器数据
	DATA.ph = PH_Check();
	DATA.tds = TDS_Check();
	DATA.flow = YFS401_GetFlowLMin() * 1000;
	DATA.turbidity = TS_GetData();
	float temp = DS18B20_Get_Temp() / 10.0f;
	if (temp > 0 && temp < 50)
	{
		DATA.temp = temp;
	}

	parseGpsBuffer();
	GPS_Data_Transfor(&DATA.lat, &DATA.lon);
}

void TIMER_Handle()
{
	if (TIMER_IT == 0)
		return;
	TIMER_IT = 0;

	if (Flagout == 0)
	{
		// 上传DATA数据
		// #模式#PH#TDS#水温#流量#浊度
		sprintf(data, "cmd=2&uid=%s&topic=data&msg=", BEMFA_ID);
		_strcat_fmt(data, "#%d", MODE);
		_strcat_fmt(data, "#%.1f", DATA.ph);
		_strcat_fmt(data, "#%.0f", DATA.tds);
		_strcat_fmt(data, "#%.1f", DATA.temp);
		_strcat_fmt(data, "#%.0f", DATA.flow);
		_strcat_fmt(data, "#%.0f", DATA.turbidity);
		_strcat_fmt(data, "#dev_data#");
		ESP8266_SendData((unsigned char *)data);

		if (DATA.lat != 0.0 && DATA.lon != 0.0)
		{
			// 上传GPS数据
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
	CONFIG.ph_h = _constrain(CONFIG.ph_h, 0, 14);
	CONFIG.tds_h = _constrain(CONFIG.tds_h, 0, 1000);
	CONFIG.temp_h = _constrain(CONFIG.temp_h, 0, 100);
	CONFIG.flow_h = _constrain(CONFIG.flow_h, 0, 100);
	CONFIG.turbidity_h = _constrain(CONFIG.turbidity_h, 0, 100);

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
}

void Beep_Beep(int duration_ms)
{
	if (DO_OP(beep, is_open))
		return;
	DO_OP(beep, open);
	delay_ms(duration_ms);
	DO_OP(beep, close);
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
	{
		TIMER_IT_Handle();
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}

// 禁用printf
int fputc(int ch, FILE *f)
{
	// 什么都不做,直接返回
	return ch;
}