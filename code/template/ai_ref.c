#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "DI.h"
#include "DS1302.h"
#include "delay.h"
#include "stdbool.h"
#include "stm32f10x.h"
#include "string.h"
#include "sys.h"

/* 系统头文件 */
#include "IO_sensor.h"
#include "Timer.h"
#include "esp8266.h"
#include "key.h"
#include "ssd1306.h"
#include "stmflash.h"
#include "utility.h"

/* 模块头文件 */
#include "MyRTC.h"
#include "dht11.h"
#include "sg_90.h"
#include "usart_driver.h"

/* AI头文件 */
#include "NanoEdgeAI_humiad.h"
#include "NanoEdgeAI_lightad.h"
#include "NanoEdgeAI_tempad.h"

/* Embedded Knowledge 配置
 * 如果导出的库包含预训练知识（knowledge_xxx.h文件），
 * 定义 USE_EMBEDDED_KNOWLEDGE 来启用快速启动模式
 */
// #define USE_EMBEDDED_KNOWLEDGE 1  // 取消注释以启用

#ifdef USE_EMBEDDED_KNOWLEDGE
#include "knowledge_humiad.h"
#include "knowledge_lightad.h"
#include "knowledge_tempad.h"
#endif

/* 工具宏定义 */
#define BTN_PAGE_SWITCH 1
#define BTN_ITEM_SWITCH 2
#define BTN_UP 3
#define BTN_DOWN 4

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
	DHT11_Data_TypeDef dht11;  // DHT11数据
	float              light;  // 光照强度

	/* AI置信度 */
	uint8_t temp_similarity;
	uint8_t humi_similarity;
	uint8_t light_similarity;
} DATA = {0};

// 配置结构体 存放阈值 保存到Flash
struct Config
{
	uint8_t temp_similarity_l;   // 温度置信度下限
	uint8_t humi_similarity_l;   // 湿度置信度下限
	uint8_t light_similarity_l;  // 光照置信度下限
} CONFIG = {
    .temp_similarity_l = 90,
    .humi_similarity_l = 90,
    .light_similarity_l = 90,
};

// 事件枚举
enum EventCode
{
	EVENT_NORMAL = 0,
	EVENT_TEMP_ANOMALY,
	EVENT_HUMI_ANOMALY,
	EVENT_LIGHT_ANOMALY,
	EVENT_CODE_MAX,
};

typedef struct
{
	enum EventCode code;
	char           str[32];
	bool           isUpload;
} Event_t;
Event_t EVENT_VAL[EVENT_CODE_MAX] = {
    {EVENT_NORMAL, "正常", true},
    {EVENT_TEMP_ANOMALY, "温度异常", true},
    {EVENT_HUMI_ANOMALY, "湿度异常", true},
    {EVENT_LIGHT_ANOMALY, "光照异常", true},
};

struct EventManager_t
{
	Event_t cur;   // 当前事件
	Event_t last;  // 上一个事件
} EVENT = {
    // 启动时强制触发一次事件处理
    .last = {EVENT_CODE_MAX, "", false},
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
    Page_test,
    Page_control,
    Page_settings,
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
DOSensorDevice *beep;
DOSensorDevice *fan;
DOSensorDevice *led;
AISensorDevice *light;
DHT11_Device   *dht11;
sg90_handle_t   sg90;
USART_handle_t  usart;

// AI缓冲区
float input_buf_tempad[DATA_INPUT_USER_TEMPAD * AXIS_NUMBER_TEMPAD];
float input_buf_humiad[DATA_INPUT_USER_HUMIAD * AXIS_NUMBER_HUMIAD];
float input_buf_lightad[DATA_INPUT_USER_LIGHTAD * AXIS_NUMBER_LIGHTAD];

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
void Time_min_add(_calendar_obj *time, int8_t add);

void neai_learn_tempad()
{
	for (int i = 0; i < MINIMUM_ITERATION_CALLS_FOR_EFFICIENT_LEARNING_TEMPAD; i++)
	{
		memset(input_buf_tempad, 0, sizeof(input_buf_tempad));
		for (int j = 0; j < DATA_INPUT_USER_TEMPAD; j++)
		{
			Read_DHT11(dht11, &DATA.dht11);
			input_buf_tempad[j] = DATA.dht11.temp_int + DATA.dht11.temp_deci / 10.0f;
		}
		neai_anomalydetection_learn_tempad(input_buf_tempad);
	}
}

void neai_learn_humiad()
{
	for (int i = 0; i < MINIMUM_ITERATION_CALLS_FOR_EFFICIENT_LEARNING_HUMIAD; i++)
	{
		memset(input_buf_humiad, 0, sizeof(input_buf_humiad));
		for (int j = 0; j < DATA_INPUT_USER_HUMIAD; j++)
		{
			Read_DHT11(dht11, &DATA.dht11);
			input_buf_humiad[j] = DATA.dht11.humi_int + DATA.dht11.humi_deci / 10.0f;
		}
		neai_anomalydetection_learn_humiad(input_buf_humiad);
	}
}
void neai_learn_lightad()
{
	for (int i = 0; i < MINIMUM_ITERATION_CALLS_FOR_EFFICIENT_LEARNING_LIGHTAD; i++)
	{
		memset(input_buf_lightad, 0, sizeof(input_buf_lightad));
		for (int j = 0; j < DATA_INPUT_USER_LIGHTAD; j++)
		{
			input_buf_lightad[j] = 100 - IO_OP(light, getPercentValue);
		}
		neai_anomalydetection_learn_lightad(input_buf_lightad);
	}
}

void light_check()
{
	memset(input_buf_lightad, 0, sizeof(input_buf_lightad));
	for (int j = 0; j < DATA_INPUT_USER_LIGHTAD; j++)
	{
		input_buf_lightad[j] = 100 - IO_OP(light, getPercentValue);
	}
	neai_anomalydetection_detect_lightad(input_buf_lightad, &DATA.light_similarity);
}
void temp_check()
{
	memset(input_buf_humiad, 0, sizeof(input_buf_humiad));
	for (int j = 0; j < DATA_INPUT_USER_HUMIAD; j++)
	{
		input_buf_humiad[j] = DATA.dht11.humi_int + DATA.dht11.humi_deci / 10.0f;
	}
	neai_anomalydetection_detect_humiad(input_buf_humiad, &DATA.humi_similarity);
}
void humi_check()
{
	memset(input_buf_tempad, 0, sizeof(input_buf_tempad));
	for (int j = 0; j < DATA_INPUT_USER_TEMPAD; j++)
	{
		input_buf_tempad[j] = DATA.dht11.temp_int + DATA.dht11.temp_deci / 10.0f;
	}
	neai_anomalydetection_detect_tempad(input_buf_tempad, &DATA.temp_similarity);
}

void neai_all_learn()
{
	oled_clear_screen(0);
	oled_draw_text_line(1, 0, (uint8_t *)"Temp learning...", NORMAL);
	oled_refresh_gram();
	neai_learn_tempad();
	oled_clear_screen(0);
	oled_draw_text_line(1, 0, (uint8_t *)"Humi learning...", NORMAL);
	oled_refresh_gram();
	neai_learn_humiad();
	oled_clear_screen(0);
	oled_draw_text_line(1, 0, (uint8_t *)"Light learning...", NORMAL);
	oled_refresh_gram();
	neai_learn_lightad();

	oled_clear_screen(0);
	oled_draw_text_line(1, 0, (uint8_t *)"AI training done", NORMAL);
	oled_refresh_gram();
	oled_clear_screen(0);
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
	beep = DOSensor_Create(GPIOC, GPIO_Pin_13, IO_HIGH);
	fan = DOSensor_Create(GPIOB, GPIO_Pin_15, IO_HIGH);
	led = DOSensor_Create(GPIOB, GPIO_Pin_1, IO_HIGH);
	light = AISensor_Create(GPIOA, GPIO_Pin_1, ADC1);
	dht11 = DHT11_Create(GPIOA, GPIO_Pin_11);
	sg90 = SG90_Create(GPIOA, GPIO_Pin_6);
	// usart = USART_Create(USART2, 115200);

	/* 模块初始化 */
	MyRTC_Init();

	/* 模式初始化 */
	Switch_Mode(MANUAL);
	LOAD_config();

	/* AI初始化 */
	_assert(neai_anomalydetection_init_tempad() == NEAI_OK);
	_assert(neai_anomalydetection_init_humiad() == NEAI_OK);
	_assert(neai_anomalydetection_init_lightad() == NEAI_OK);

#ifdef USE_EMBEDDED_KNOWLEDGE
	/* 加载预训练知识 */
	oled_draw_text_line(0, 0, (uint8_t *)"Loading pre-trained...", NORMAL);
	oled_refresh_gram();
	_assert(neai_anomalydetection_knowledge_tempad(knowledge_tempad) == NEAI_OK);
	_assert(neai_anomalydetection_knowledge_humiad(knowledge_humiad) == NEAI_OK);
	_assert(neai_anomalydetection_knowledge_lightad(knowledge_lightad) == NEAI_OK);

	oled_clear_screen(0);
	oled_draw_text_line(0, 0, (uint8_t *)"Ready!", NORMAL);
	oled_refresh_gram();
	delay_ms(500);
#else
	/* 无预训练知识，需要完整学习 */
	neai_all_learn();
#endif

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
		if ((ptr = strstr((char *)esp8266_buf, "temp_similarity_l=")) != NULL)
		{
			sscanf(ptr, "temp_similarity_l=%hhu", &CONFIG.temp_similarity_l);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "humi_similarity_l=")) != NULL)
		{
			sscanf(ptr, "humi_similarity_l=%hhu", &CONFIG.humi_similarity_l);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "light_similarity_l=")) != NULL)
		{
			sscanf(ptr, "light_similarity_l=%hhu", &CONFIG.light_similarity_l);
			SAVE_config();
			Beep_Beep(20);
		}
		/* 设备控制 */
		if ((ptr = strstr((char *)esp8266_buf, "curtain=")) != NULL)
		{
			uint16_t state;
			sscanf(ptr, "curtain=%hu", &state);
			SG90_SetAngle(sg90, state ? 90 : 0);
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "fan=")) != NULL)
		{
			uint8_t state;
			sscanf(ptr, "fan=%hhu", &state);
			state ? IO_OP(fan, open) : IO_OP(fan, close);
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "led=")) != NULL)
		{
			uint8_t state;
			sscanf(ptr, "led=%hhu", &state);
			state ? IO_OP(led, open) : IO_OP(led, close);
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
	if (DATA.temp_similarity < CONFIG.temp_similarity_l)
	{
		EVENT_SET(EVENT.cur, EVENT_TEMP_ANOMALY);
	}
	else if (DATA.humi_similarity < CONFIG.humi_similarity_l)
	{
		EVENT_SET(EVENT.cur, EVENT_HUMI_ANOMALY);
	}
	else if (DATA.light_similarity < CONFIG.light_similarity_l)
	{
		EVENT_SET(EVENT.cur, EVENT_LIGHT_ANOMALY);
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
		case EVENT_TEMP_ANOMALY:
			IO_OP(fan, close);
			break;
		case EVENT_HUMI_ANOMALY:
			break;
		case EVENT_LIGHT_ANOMALY:
			SG90_SetAngle(sg90, 0);
			break;
		default:
			break;
		}

		// 处理当前事件
		switch (EVENT.cur.code)
		{
		case EVENT_NORMAL:
			IO_OP(beep, close);
			IO_OP(led, close);
			break;
		case EVENT_TEMP_ANOMALY:
			IO_OP(beep, open);
			IO_OP(led, open);
			IO_OP(fan, open);
			break;
		case EVENT_HUMI_ANOMALY:
			IO_OP(beep, open);
			IO_OP(led, open);
			break;
		case EVENT_LIGHT_ANOMALY:
			IO_OP(beep, open);
			IO_OP(led, open);
			SG90_SetAngle(sg90, 90);
			break;
		default:
			break;
		}
	}
}

/***
 * 界面一
 * 时间
 * 温湿度
 * 光强
 * 状态
 */
void Page_data1()
{
	/* 按键处理 */

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "%02d/%02d %02d:%02d:%02d", calendar.w_month, calendar.w_date, calendar.hour, calendar.min, calendar.sec);
	sprintf((char *)items[items_count++], "T:%d.%dC  H:%d.%d%%", DATA.dht11.temp_int, DATA.dht11.temp_deci, DATA.dht11.humi_int, DATA.dht11.humi_deci);
	sprintf((char *)items[items_count++], "光强: %.0f", DATA.light);
	sprintf((char *)items[items_count++], "状态: %s", EVENT.cur.str);

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
 * 学习
 * 风扇
 * 照明
 * 窗帘
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
			neai_all_learn();
			break;
		case 2:
			key_num == BTN_UP ? IO_OP(fan, open) : IO_OP(fan, close);
			break;
		case 3:
			key_num == BTN_UP ? IO_OP(led, open) : IO_OP(led, close);
			break;
		case 4:
			SG90_SetAngle(sg90, key_num == BTN_UP ? 90 : 0);
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
	sprintf((char *)items[items_count++], "AI学习");
	sprintf((char *)items[items_count++], "风扇: %s", IO_OP(fan, isOpen) ? "ON" : "OFF");
	sprintf((char *)items[items_count++], "照明: %s", IO_OP(led, isOpen) ? "ON" : "OFF");
	sprintf((char *)items[items_count++], "窗帘: %s", SG90_GetAngle(sg90) == 90 ? "ON" : "OFF");

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

void Page_settings()
{
	/* 按键处理 */
	if (key_num == BTN_UP || key_num == BTN_DOWN)
	{
		switch (item_idx)
		{
		case 0:
			CONFIG.temp_similarity_l += (key_num == BTN_UP) ? 5 : -5;
			break;
		case 1:
			CONFIG.humi_similarity_l += (key_num == BTN_UP) ? 5 : -5;
			break;
		case 2:
			CONFIG.light_similarity_l += (key_num == BTN_UP) ? 5 : -5;
			break;
		}
		SAVE_config();
		Beep_Beep(20);
	}

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "温度置信度: %2d", CONFIG.temp_similarity_l);
	sprintf((char *)items[items_count++], "湿度置信度: %2d", CONFIG.humi_similarity_l);
	sprintf((char *)items[items_count++], "光强置信度: %2d", CONFIG.light_similarity_l);

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
void Page_test()
{
	item_idx_min = 0;
	items_count = 0;
	// 显示所有置信度
	sprintf((char *)items[items_count++], "AI识别结果:");
	sprintf((char *)items[items_count++], "温度: %d", DATA.temp_similarity);
	sprintf((char *)items[items_count++], "湿度: %d", DATA.humi_similarity);
	sprintf((char *)items[items_count++], "光强: %d", DATA.light_similarity);

	item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	item_idx_start = _max(0, item_idx - item_max_count + 1);
	for (int i = 0; i < _min(item_max_count, items_count); i++)
	{
		oled_clear_line(i);
		oled_draw_text_line(i, 0, (uint8_t *)items[item_idx_start + i],
		                    NORMAL);
	}
	oled_refresh_gram();
	oled_clear_screen(0);
}

void SENSOR_Handle()
{
	// 读取传感器数据
	Read_DHT11(dht11, &DATA.dht11);
	DATA.light = 100 - IO_OP(light, getPercentValue);
	temp_check();
	humi_check();
	light_check();
}

void TIMER_Handle()
{
	if (TIMER_IT == 0)
		return;
	TIMER_IT = 0;

	// RTC_Get();
	MyRTC_ReadTime();

	if (Flagout == 0)
	{
		// 上传DATA数据
		// #模式#温度#湿度#光强#风扇#照明#窗帘
		sprintf(data, "cmd=2&uid=%s&topic=data&msg=", BEMFA_ID);
		_strcat_fmt(data, "#%d", MODE);
		_strcat_fmt(data, "#%d.%d", DATA.dht11.temp_int, DATA.dht11.temp_deci);
		_strcat_fmt(data, "#%d.%d", DATA.dht11.humi_int, DATA.dht11.humi_deci);
		_strcat_fmt(data, "#%.0f", DATA.light);
		_strcat_fmt(data, "#%s", IO_OP(fan, isOpen) ? "ON" : "OFF");
		_strcat_fmt(data, "#%s", IO_OP(led, isOpen) ? "ON" : "OFF");
		_strcat_fmt(data, "#%s", SG90_GetAngle(sg90) == 90 ? "ON" : "OFF");
		_strcat_fmt(data, "#dev_data#");
		ESP8266_SendData((unsigned char *)data);
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
	CONFIG.temp_similarity_l = _constrain(CONFIG.temp_similarity_l, 0, 100);
	CONFIG.humi_similarity_l = _constrain(CONFIG.humi_similarity_l, 0, 100);
	CONFIG.light_similarity_l = _constrain(CONFIG.light_similarity_l, 0, 100);

	STMFLASH_WriteBuf(FLASH_SAVE_ADDR, &CONFIG, sizeof(CONFIG));
}

void LOAD_config()
{
	STMFLASH_ReadBuf(FLASH_SAVE_ADDR, &CONFIG, sizeof(CONFIG));
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
	IO_OP(beep, close);
	IO_OP(led, close);
	IO_OP(fan, close);
	SG90_SetAngle(sg90, 0);
}

void Beep_Beep(int duration_ms)
{
	if (IO_OP(beep, isOpen))
		return;
	IO_OP(beep, open);
	delay_ms(duration_ms);
	IO_OP(beep, close);
}

void Time_min_add(_calendar_obj *time, int8_t add)
{
	time->min += add;
	if (time->min < 0)
	{
		time->hour -= 1;
		time->min = 59;
	}
	else if (time->min > 59)
	{
		time->hour += 1;
		time->min = 0;
	}

	if (time->hour < 0)
	{
		time->hour = 23;
	}
	else if (time->hour > 23)
	{
		time->hour = 0;
	}

	if (time->w_date < 1)
	{
		time->w_date = 31;
	}
	else if (time->w_date > 31)
	{
		time->w_date = 1;
	}

	if (time->w_month < 1)
	{
		time->w_month = 12;
	}
	else if (time->w_month > 12)
	{
		time->w_month = 1;
	}

	if (time->w_year < 2000)
	{
		time->w_year = 2000;
	}
	else if (time->w_year > 2099)
	{
		time->w_year = 2000;
	}
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