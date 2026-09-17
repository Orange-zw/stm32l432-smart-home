#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "DO.h"
#include "delay.h"
#include "stm32f10x.h"
#include "string.h"
#include "sys.h"

/* 系统头文件 */
#include "GUI.h"
#include "IO_sensor.h"
#include "Timer.h"
#include "esp8266.h"
#include "key.h"
#include "stmflash.h"
#include "utility.h"

/* 模块头文件 */
#include "MyRTC.h"
#include "dht11.h"
#include "usart_driver.h"

/* 工具宏定义 */
#define BTN_PAGE_SWITCH 1
#define BTN_ITEM_SWITCH 2
#define BTN_UP 3
#define BTN_DOWN 4

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
	float temp;     // 温度
	float humi;     // 湿度
	float light;    // 光照强度
	float mq2_gas;  // MQ-2气体浓度
} DATA = {0};

// 配置结构体 存放阈值 保存到Flash
struct Config
{
	uint16_t temp_h;   // 温度上限
	uint16_t humi_h;   // 湿度上限
	float    mq2_h;    // 气体浓度上限
	float    light_l;  // 光照强度下限
} CONFIG = {0};

// 事件枚举
enum EventCode
{
	EVENT_NORMAL = 0,
	EVENT_TEMP_HIGH,
	EVENT_HUMI_HIGH,
	EVENT_GAS_HIGH,
	EVENT_LIGHT_LOW,
	EVENT_CODE_MAX,
};

struct Evet_t
{
	enum EventCode code;
	enum EventCode last_code;
	struct
	{
		bool  isUpload;
		char *str;
	} VAL[EVENT_CODE_MAX];
} EVENT = {
    .code = EVENT_NORMAL,
    .last_code = EVENT_CODE_MAX,  // 初始化为无效值 强制触发一次事件处理
    .VAL = {
        {true, "正常        "},
        {true, "温度过高    "},
        {true, "湿度过高    "},
        {true, "烟雾浓度过高"},
        {true, "光照强度过低"},
    },
};

// 系统运行时间
uint64_t RUN_TIME = 0, LAST_TIME = 0;
// OLED显示
char oled_str[100];
// 按键状态
u8 key_num = 0;
// ESP8266相关
extern unsigned short esp8266_cnt;
char                  TIMER_IT = 0, Flagout = 1;
char                  data[200];
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
    // Page_data2,
    // Page_control,
    Page_settings,
    // Page_test,
};
uint8_t page_count = sizeof(PAGES) / sizeof(PAGES[0]);
uint8_t page_idx = 0;
uint8_t item_idx_min = 0;
uint8_t item_max_count = 8;
uint8_t items[10][256];
uint8_t items_count = 0;
uint8_t item_idx = 0;
uint8_t item_idx_start = 0;

// 设备指针
DO_handle_t    beep;
USART_handle_t lora_uart;

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

int main(void)
{
	/* 系统默认初始化 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	SystemInit();
	Key_Init();
	Timer_Init(72, 1000);  // 72MHz时钟，预分频72，周期1000 -> 1kHz -> 1ms中断
	/* 设备初始化 */
	beep = DO_Create(GPIOC, GPIO_Pin_13, IO_HIGH);
	lora_uart = USART_Create(USART1, 115200);
	/* 模块初始化 */
	MyRTC_Init();

	/* 模式初始化 */
	Switch_Mode(MANUAL);
	LOAD_config();

	LCD_Init();
	LCD_Clear(0);
	do
	{
		key_num = KEY_Scan(0);  // 按键扫描
		Show_Str(8, 32, YELLOW, BLACK, (u8 *)"请选择是否联网", 16, 0);
		Show_Str(24, 64, YELLOW, BLACK, (u8 *)"1.是  2.否", 16, 0);
		if (key_num == 1)
		{
			LCD_Clear(0);
			ESP8266_Init(115200);
			break;
		}
		else if (key_num == 2)
		{
			LCD_Clear(0);
			LCD_Clear(BLACK);
			break;
		}
	} while (1);
	LCD_Clear(0);
	MyRTC_ReadTime();
	USART_OP(lora_uart, send_fmt, "sys_time=%d-%d-%d %d:%d:%d", calendar.w_year, calendar.w_month, calendar.w_date, calendar.hour, calendar.min, calendar.sec);

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

	if (USART_OP(lora_uart, is_rx_ready))
	{
		USART_OP(lora_uart, get_rx_data, rx_data, sizeof(rx_data));

		/* 参数设置 */
		// #温度#湿度#光照#烟雾浓度#dev_data#
		if ((ptr = strstr((char *)rx_data, "dev_data")) != NULL)
		{
			float temp, humi, light, mq2_gas;
			sscanf((char *)rx_data, "#%f#%f#%f#%f#dev_data#", &temp, &humi, &light, &mq2_gas);
			DATA.temp = temp;
			DATA.humi = humi;
			DATA.light = light;
			DATA.mq2_gas = mq2_gas;
			// Beep_Beep(20);
		}

		/* 设备控制 */

		/* 模式切换 */

		/* 清除缓存 */
		USART_OP(lora_uart, flush_rx);
	}
	if (esp8266_cnt > 0)
	{
		/* 参数设置 */
		if ((ptr = strstr((char *)esp8266_buf, "temp_h=")) != NULL)
		{
			uint16_t temp_h;
			sscanf(ptr, "temp_h=%hu", &temp_h);
			CONFIG.temp_h = temp_h;
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "humi_h=")) != NULL)
		{
			uint16_t humi_h;
			sscanf(ptr, "humi_h=%hu", &humi_h);
			CONFIG.humi_h = humi_h;
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "mq2_h=")) != NULL)
		{
			float mq2_h;
			sscanf(ptr, "mq2_h=%f", &mq2_h);
			CONFIG.mq2_h = mq2_h;
			SAVE_config();
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "light_l=")) != NULL)
		{
			float light_l;
			sscanf(ptr, "light_l=%f", &light_l);
			CONFIG.light_l = light_l;
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
			Beep_Beep(20);
		}

		/* 清除缓存 */
		ESP8266_Clear();
	}
}

void AUTO_Handle()
{
	// 单次只能处理一个事件，优先级从高到低
	if (DATA.temp > CONFIG.temp_h)
	{
		EVENT.code = EVENT_TEMP_HIGH;
	}
	else if (DATA.humi > CONFIG.humi_h)
	{
		EVENT.code = EVENT_HUMI_HIGH;
	}
	else if (DATA.mq2_gas > CONFIG.mq2_h)
	{
		EVENT.code = EVENT_GAS_HIGH;
	}
	else if (DATA.light < CONFIG.light_l && DATA.light > 0)
	{
		EVENT.code = EVENT_LIGHT_LOW;
	}
	else
	{
		EVENT.code = EVENT_NORMAL;
	}
}

void EVENT_Handle()
{
	if (EVENT.code != EVENT.last_code)
	{
		enum EventCode last_code = EVENT.last_code;
		EVENT.last_code = EVENT.code;
		if (EVENT.VAL[EVENT.code].isUpload)
		{
			if (Flagout == 0)
			{
				sprintf(data, "cmd=2&uid=%s&topic=event&msg=$%d$event_data$", BEMFA_ID, EVENT.code);
				ESP8266_SendData((uint8_t *)data);
			}

			USART_OP(lora_uart, send_fmt, "event=%d", EVENT.code);
		}
		// 处理上一个事件的善后
		switch (last_code)
		{
		case EVENT_TEMP_HIGH:
		case EVENT_HUMI_HIGH:
		case EVENT_LIGHT_LOW:
		case EVENT_GAS_HIGH:
			IO_OP(beep, close);
			break;
		}

		// 处理当前事件
		switch (EVENT.code)
		{
		case EVENT_NORMAL:  // 正常
			IO_OP(beep, close);
			break;
		case EVENT_TEMP_HIGH:  // 温度过高
			IO_OP(beep, open);
			break;
		case EVENT_HUMI_HIGH:  // 湿度过高
			IO_OP(beep, open);
			break;
		case EVENT_GAS_HIGH:  // 气体浓度过高
			IO_OP(beep, open);
			break;
		case EVENT_LIGHT_LOW:  // 光照强度过低
			IO_OP(beep, open);
			break;
		default:
			break;
		}
	}
}

/***
 * 数据界面一
 * 时间
 * 温湿度
 * 状态
 */
void Page_data1()
{
	/* 按键处理 */

	/* 页面项目 */
	item_idx_min = 0;
	sprintf((char *)items[items_count++], "%02d/%02d %02d:%02d:%02d", calendar.w_month, calendar.w_date, calendar.hour, calendar.min, calendar.sec);
	sprintf((char *)items[items_count++], "T:%.1f℃  H:%.1f%%", DATA.temp, DATA.humi);
	sprintf((char *)items[items_count++], "光照: %.0f", DATA.light);
	sprintf((char *)items[items_count++], "烟雾浓度: %.0f", DATA.mq2_gas);
	sprintf((char *)items[items_count++], "状态: %s", EVENT.VAL[EVENT.code].str);

	/* 页面显示 */
	item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	item_idx_start = _max(0, item_idx - 3);
	for (int i = 0; i < _min(item_max_count, items_count); i++)
	{
		Show_Str(0, i * 20, YELLOW, BLACK, (u8 *)items[item_idx_start + i], 16, 0);
	}
	items_count = 0;
}

/**
 * 控制界面
 * 模式
 * 场景
 * 加热
 * LED
 * 水泵
 */
void Page_control()
{
	/* 按键处理 */
	if (key_num == BTN_UP || key_num == BTN_DOWN)
	{
		switch (item_idx)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		default:
			break;
		}
		SAVE_config();
		Beep_Beep(20);
	}

	/* 页面项目 */
	item_idx_min = false;
	sprintf((char *)items[items_count++], "工作模式: %s", MODE_STR[MODE]);
	sprintf((char *)items[items_count++], "光照: %.0f", DATA.light);
	sprintf((char *)items[items_count++], "烟雾浓度: %.0f", DATA.mq2_gas);

	/* 页面显示 */
	item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	item_idx_start = _max(0, item_idx - 3);

	for (int i = 0; i < _min(item_max_count, items_count); i++)
	{
		Show_Str(0, i * 20, YELLOW, BLACK, (u8 *)items[item_idx_start + i], 16, 0);
	}
	items_count = 0;
}

/**
 * 设置界面
 *
 * 温度上限
 * 湿度上限
 * 光强上限
 * 烟雾浓度上限
 */
void Page_settings()
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
			CONFIG.temp_h += key_num == BTN_UP ? 1 : -1;
			break;
		case 2:
			CONFIG.humi_h += key_num == BTN_UP ? 1 : -1;
			break;
		case 3:
			CONFIG.mq2_h += key_num == BTN_UP ? 5.0f : -5.0f;
			break;
		case 4:
			CONFIG.light_l += key_num == BTN_UP ? 5.0f : -5.0f;
			break;
		default:
			break;
		}
		SAVE_config();
		Beep_Beep(20);
	}

	/* 页面项目 */
	item_idx_min = 0;
	sprintf((char *)items[items_count++], "工作模式: %s", MODE_STR[MODE]);
	sprintf((char *)items[items_count++], "温度上限: %2d ℃", CONFIG.temp_h);
	sprintf((char *)items[items_count++], "湿度上限: %2d %%", CONFIG.humi_h);
	sprintf((char *)items[items_count++], "烟雾浓度上限: %2.0f", CONFIG.mq2_h);
	sprintf((char *)items[items_count++], "光照强度上限: %2.0f", CONFIG.light_l);

	/* 页面显示 */
	item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	item_idx_start = _max(0, item_idx - item_max_count + 1);
	for (int i = 0; i < _min(item_max_count, items_count); i++)
	{
		uint16_t bg_color = item_idx_start + i == item_idx ? YELLOW : BLACK;
		uint16_t text_color = item_idx_start + i == item_idx ? BLACK : YELLOW;
		Show_Str(0, i * 20, text_color, bg_color, (u8 *)items[item_idx_start + i], 16, 0);
	}
	items_count = 0;
}

void SENSOR_Handle()
{
	// 读取传感器数据
}

void TIMER_Handle()
{
	if (TIMER_IT == 0)
		return;
	TIMER_IT = 0;

	MyRTC_ReadTime();

	if (Flagout == 0)
	{
		// 上传DATA数据
		// #模式#温度#湿度#光照#烟雾浓度
		memset(data, 0, sizeof(data));
		sprintf(data, "cmd=2&uid=%s&topic=data&msg=", BEMFA_ID);
		_strcat_fmt(data, "#%d", MODE);
		_strcat_fmt(data, "#%.1f", DATA.temp);
		_strcat_fmt(data, "#%.1f", DATA.humi);
		_strcat_fmt(data, "#%.1f", DATA.light);
		_strcat_fmt(data, "#%.1f", DATA.mq2_gas);
		_strcat_fmt(data, "#dev_data#");

		ESP8266_SendData((uint8_t *)data);
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
		LCD_Clear(0);
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
	CONFIG.mq2_h = _constrain(CONFIG.mq2_h, 0, 100);
	CONFIG.light_l = _constrain(CONFIG.light_l, 0, 100);
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
	EVENT.code = EVENT_NORMAL;
	MODE = new_mode;

	// 切换到模式时关闭所有设备
	IO_OP(beep, close);
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