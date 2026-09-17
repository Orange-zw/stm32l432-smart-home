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
#include "esp8266.h"
#include "key.h"
#include "ssd1306.h"
#include "stmflash.h"
#include "utility.h"

/* 模块头文件 */
#include "JW01.h"
#include "MyRTC.h"
#include "dht11.h"
#include "pwm_driver.h"
#include "usart_driver.h"

/* 工具宏定义 */
#define BTN_PAGE_SWITCH 1
#define BTN_ITEM_SWITCH 2
#define BTN_UP 3
#define BTN_DOWN 4

#define CONFIG_FLAG (0x125678)
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
	float              co2;
	float              mq2;

	int8_t  fan_gear;           // 风扇档位
	uint8_t fan_gear_value[4];  // 风扇档位值
} DATA = {
    .fan_gear_value = {0, 40, 70, 100},
};
const char *FAN_GEAR_STR[4] = {
    "关闭",
    "低档",
    "中档",
    "高档",
};

// 配置结构体 存放阈值 保存到Flash
struct Config
{
	const uint32_t flag;

	uint8_t temp_h;  // 温度上限
	uint8_t humi_h;  // 湿度上限
	float   co2_h;   // CO2浓度上限
	float   mq2_h;   // MQ-2气体浓度上限
} CONFIG = {
    .flag = CONFIG_FLAG,
    .temp_h = 40,
    .humi_h = 80,
    .co2_h = 600.0f,
    .mq2_h = 70.0f,
};

// 事件枚举
enum EventCode
{
	EVENT_NORMAL = 0,
	EVENT_TEMP_HIGH,
	EVENT_HUMI_HIGH,
	EVENT_CO2_HIGH,
	EVENT_MQ2_HIGH,
	EVENT_CODE_MAX,
};

typedef struct
{
	char           str[32];
	enum EventCode code;
	bool           isUpload;
} Event_t;
Event_t EVENT_VAL[EVENT_CODE_MAX] = {
    {"正常    ", EVENT_NORMAL, true},
    {"温度过高", EVENT_TEMP_HIGH, true},
    {"湿度过高", EVENT_HUMI_HIGH, true},
    {"CO2 过高", EVENT_CO2_HIGH, true},
    {"MQ-2过高", EVENT_MQ2_HIGH, true},
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
AISensorDevice *mq2_sensor;
DHT11_Device   *dht11;
PWM_handle_t    fan;
USART_handle_t  bt_uart;

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
void fan_control(int8_t state)
{
	DATA.fan_gear = _constrain(DATA.fan_gear + state, 0, 3);
	PWM_OP(fan, setDutyCycle, DATA.fan_gear_value[DATA.fan_gear]);
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
	mq2_sensor = AISensor_Create(GPIOA, GPIO_Pin_5, ADC1);
	fan = PWM_Create(GPIOA, GPIO_Pin_6);
	dht11 = DHT11_Create(GPIOA, GPIO_Pin_11);
	bt_uart = USART_Create(USART3, 9600);

	/* 模块初始化 */
	MyRTC_Init();
	JW01_Init(9600);

	/* 模式初始化 */
	Switch_Mode(MANUAL);
	LOAD_config();

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
	uint8_t rx_data[128];
	char   *ptr;

	if (USART_OP(bt_uart, is_rx_ready))
	{
		USART_OP(bt_uart, get_rx_data, rx_data, sizeof(rx_data));

		/* 参数设置 */
		if ((ptr = strstr((char *)rx_data, "temp_h=")) != NULL)
		{
			sscanf(ptr, "temp_h=%hhu", &CONFIG.temp_h);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)rx_data, "humi_h=")) != NULL)
		{
			sscanf(ptr, "humi_h=%hhu", &CONFIG.humi_h);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)rx_data, "co2_h=")) != NULL)
		{
			sscanf(ptr, "co2_h=%f", &CONFIG.co2_h);
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)rx_data, "mq2_h=")) != NULL)
		{
			sscanf(ptr, "mq2_h=%f", &CONFIG.mq2_h);
			SAVE_config();
			Beep_Beep(20);
		}

		/* 设备控制 */
		if ((ptr = strstr((char *)rx_data, "fan=")) != NULL)
		{
			int state;
			sscanf(ptr, "fan=%d", &state);
			fan_control(state);
			Beep_Beep(20);
		}

		/* 模式切换 */
		if ((ptr = strstr((char *)rx_data, "mode=")) != NULL)
		{
			uint8_t mode;
			sscanf(ptr, "mode=%hhu", &mode);

			Switch_Mode((enum Mode)mode);
			SAVE_config();
			Beep_Beep(20);
		}

		/* 清除缓存 */
		USART_OP(bt_uart, flush_rx);
	}
}

// 根据CO2和烟雾浓度自动计算风扇档位（仅自动模式调用）
static void AUTO_Fan_Control(void)
{
	// 档位映射：浓度 level 对应档位的阈值 [0.50, 0.70, 0.90]
	// <50%→0关 <70%→1低档 <90%→2中档 >=90%→3高档
	const float FAN_GEAR_LEVEL[3] = {0.50f, 0.70f, 0.90f};

	float co2_ratio = (CONFIG.co2_h > 0) ? (DATA.co2 / CONFIG.co2_h) : 0;
	float mq2_ratio = (CONFIG.mq2_h > 0) ? (DATA.mq2 / CONFIG.mq2_h) : 0;
	if (co2_ratio > 1.0f) co2_ratio = 1.0f;
	if (mq2_ratio > 1.0f) mq2_ratio = 1.0f;

	// 取两者中较高者，浓度越高档位越高
	float level = (co2_ratio > mq2_ratio) ? co2_ratio : mq2_ratio;

	// 根据 FAN_GEAR_LEVEL 数组映射到档位
	int8_t target_gear = 0;
	for (int i = 0; i < 3; i++)
	{
		if (level >= FAN_GEAR_LEVEL[i])
			target_gear = i + 1;
		else
			break;
	}

	if (DATA.fan_gear != target_gear)
	{
		DATA.fan_gear = target_gear;
		PWM_OP(fan, setDutyCycle, DATA.fan_gear_value[DATA.fan_gear]);
	}
}

void AUTO_Handle()
{
	// 烟雾/CO2浓度越高风扇档位越高，越低档位越低
	AUTO_Fan_Control();

	// 单次只能处理一个事件，优先级从高到低
	if (DATA.dht11.temp_int > CONFIG.temp_h)
	{
		EVENT_SET(EVENT.cur, EVENT_TEMP_HIGH);
	}
	else if (DATA.dht11.humi_int > CONFIG.humi_h)
	{
		EVENT_SET(EVENT.cur, EVENT_HUMI_HIGH);
	}
	else if (DATA.co2 > CONFIG.co2_h)
	{
		EVENT_SET(EVENT.cur, EVENT_CO2_HIGH);
	}
	else if (DATA.mq2 > CONFIG.mq2_h)
	{
		EVENT_SET(EVENT.cur, EVENT_MQ2_HIGH);
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

		enum EventCode code = EVENT_NORMAL;
		if (EVENT.cur.isUpload)
		{
			code = EVENT.cur.code;
		}
		sprintf(data, "cmd=2&uid=%s&topic=event&msg=$%d$event_data$", BEMFA_ID, code);
		USART_OP(bt_uart, send, (unsigned char *)data, strlen(data));

		// 处理上一个事件的善后
		switch (last_event.code)
		{
		case EVENT_TEMP_HIGH:
			IO_OP(beep, close);
			break;
		case EVENT_HUMI_HIGH:
			IO_OP(beep, close);
			break;
		case EVENT_CO2_HIGH:
			IO_OP(beep, close);
			break;
		case EVENT_MQ2_HIGH:
			IO_OP(beep, close);
			break;
		default:
			break;
		}

		// 处理当前事件
		switch (EVENT.cur.code)
		{
		case EVENT_NORMAL:
			IO_OP(beep, close);
			break;
		case EVENT_TEMP_HIGH:
			IO_OP(beep, open);
			break;
		case EVENT_HUMI_HIGH:
			IO_OP(beep, open);
			break;
		case EVENT_CO2_HIGH:
			IO_OP(beep, open);
			break;
		case EVENT_MQ2_HIGH:
			IO_OP(beep, open);
			break;
		default:
			break;
		}
	}
}

/***
 * 界面一
 * B22040702sxn
 * 温湿度
 * MQ2
 * CO2
 */
void Page_data1()
{
	/* 按键处理 */

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	sprintf((char *)items[items_count++], "B22040702sxn");
	sprintf((char *)items[items_count++], "T:%d.%dC  H:%d.%d%%", DATA.dht11.temp_int, DATA.dht11.temp_deci, DATA.dht11.humi_int, DATA.dht11.humi_deci);
	sprintf((char *)items[items_count++], "CO2 : %.0f ppm", DATA.co2);
	sprintf((char *)items[items_count++], "烟雾: %.0f", DATA.mq2);

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
 * 状态
 * 模式
 * 风扇：关闭/低档/中档/高档
 */
void Page_control()
{
	/* 按键处理 */
	if (key_num == BTN_UP || key_num == BTN_DOWN)
	{
		switch (item_idx)
		{
		case 1:
			Switch_Mode(key_num == BTN_UP ? AUTO : MANUAL);
			break;
		case 2:
			fan_control(key_num == BTN_UP ? 1 : -1);
			break;
		default:
			break;
		}
		SAVE_config();
		Beep_Beep(20);
	}

	/* 页面项目 */
	item_idx_min = 1;
	items_count = 0;
	sprintf((char *)items[items_count++], "状态: %s", EVENT.cur.str);
	sprintf((char *)items[items_count++], "模式: %s", MODE_STR[MODE]);
	sprintf((char *)items[items_count++], "风扇: %s", FAN_GEAR_STR[DATA.fan_gear]);

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
 * 界面四
 * 温度上限
 * 湿度上限
 * CO2上限
 * MQ2上限
 */
void Page_settings()
{
	/* 按键处理 */
	if (key_num == BTN_UP || key_num == BTN_DOWN)
	{
		switch (item_idx)
		{
		case 0:
			CONFIG.temp_h += (key_num == BTN_UP) ? 1 : -1;
			break;
		case 1:
			CONFIG.humi_h += (key_num == BTN_UP) ? 5 : -5;
			break;
		case 2:
			CONFIG.mq2_h += (key_num == BTN_UP) ? 5.0f : -5.0f;
			break;
		case 3:
			CONFIG.co2_h += (key_num == BTN_UP) ? 50.0f : -50.0f;
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
	sprintf((char *)items[items_count++], "温度上限: %d", CONFIG.temp_h);
	sprintf((char *)items[items_count++], "湿度上限: %d", CONFIG.humi_h);
	sprintf((char *)items[items_count++], "烟雾上限: %.0f", CONFIG.mq2_h);
	sprintf((char *)items[items_count++], "CO2上限: %.0f", CONFIG.co2_h);

	/* 页面显示 */
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

void SENSOR_Handle()
{
	// 读取传感器数据
	Read_DHT11(dht11, &DATA.dht11);
	DATA.co2 = JW01_GetCO2();
	DATA.mq2 = IO_OP(mq2_sensor, getPercentValue);
}

void TIMER_Handle()
{
	if (TIMER_IT == 0)
		return;
	TIMER_IT = 0;

	MyRTC_ReadTime();

	// 上传DATA数据
	// #模式#温度#湿度#CO2#MQ2#风扇档位
	sprintf(data, "cmd=2&uid=%s&topic=data&msg=", BEMFA_ID);
	_strcat_fmt(data, "#%d", MODE);
	_strcat_fmt(data, "#%d.%d", DATA.dht11.temp_int, DATA.dht11.temp_deci);
	_strcat_fmt(data, "#%d.%d", DATA.dht11.humi_int, DATA.dht11.humi_deci);
	_strcat_fmt(data, "#%.0f", DATA.co2);
	_strcat_fmt(data, "#%.0f", DATA.mq2);
	_strcat_fmt(data, "#%s", FAN_GEAR_STR[DATA.fan_gear]);
	_strcat_fmt(data, "#dev_data#");
	USART_OP(bt_uart, send, (unsigned char *)data, strlen(data));
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
	CONFIG.temp_h = _constrain(CONFIG.temp_h, 0, 100);
	CONFIG.humi_h = _constrain(CONFIG.humi_h, 0, 100);
	CONFIG.co2_h = _constrain(CONFIG.co2_h, 0, 10000);
	CONFIG.mq2_h = _constrain(CONFIG.mq2_h, 0, 100);

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
	IO_OP(beep, close);
	DATA.fan_gear = 0;
	PWM_OP(fan, setDutyCycle, DATA.fan_gear_value[DATA.fan_gear]);
}

void Beep_Beep(int duration_ms)
{
	if (IO_OP(beep, isOpen))
		return;
	IO_OP(beep, open);
	delay_ms(duration_ms);
	IO_OP(beep, close);
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