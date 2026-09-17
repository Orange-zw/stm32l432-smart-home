/* ============================================================
 * 模块     : 主程序
 * 功能     : 系统初始化、三控制入口命令处理、页面管理与云端上报
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "AI.h"
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
#include "oled.h"
#include "sg_90.h"
#include "pwm_driver.h"
#include "su_03t.h"

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
    "语音",
};

/**
 *  数据结构体
 *
 * */
// struct Data
// {



// } DATA = {0};

// 配置结构体 存放阈值 保存到Flash
struct Config
{
	const uint32_t flag;


} CONFIG = {
    .flag = CONFIG_FLAG,


};

// 事件枚举
enum EventCode
{
	EVENT_NORMAL = 0,

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
char data[250];

// 页面管理
void Page_data1(void);
void Page_data2(void);
void Page_data3(void);
void Page_control(void);
void Page_settings(void);

void Page_HeartRate(void);
void (*PAGES[])(void) = {
    //Page_data1,
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
DO_handle_t   beep;
DO_handle_t   led_white;
DO_handle_t   led_red;
DO_handle_t   led_blue;
DO_handle_t   led_green;
PWM_Handle_t    *fan;
sg90_handle_t door;

// 风扇档位变量：允许 0~3 档（0档=关闭）
uint8_t fan_flag = 0;
const uint8_t FAN_DUTY_LEVEL[4] = {0, 35, 70, 100};
const char *FAN_LEVEL_STR[4] = {
	"0档",
	"1档",
	"2档",
	"3档",
};

// 空调状态：0关闭 1制热(红) 2制冷(蓝)
uint8_t ac_flag = 0;
const char *AC_STATE_STR[3] = {
	"关闭",
	"制热",
	"制冷",
};

/* 函数声明 */
void Switch_Mode(enum Mode new_mode);
void SAVE_config();
void LOAD_config();
void MANUAL_Handle();
void AUDIO_Handle();
//void AUTO_Handle();
void APP_Handle();
//void EVENT_Handle();
void SENSOR_Handle();
void KEY_Handle();
void TIMER_Handle();     // 定时器处理函数 低实时
void TIMER_IT_Handle();  // 定时器处理函数 中断调用 高实时

// 业务函数
void Beep_Beep(int duration_ms);
void Door_control(bool state)
{
	state ? SG90_SetAngle(door, 90) : SG90_SetAngle(door, 0);
}
void AC_control(int8_t state)
{
	ac_flag = _constrain(state, 0, 2);

	// 先全部关闭，确保不会出现制热和制冷同时亮
	DO_OP(led_red, close);
	DO_OP(led_blue, close);

	if (ac_flag == 1)
	{
		DO_OP(led_red, open);
	}
	else if (ac_flag == 2)
	{
		DO_OP(led_blue, open);
	}
}
void fan_set_gear(int8_t gear)
{
	fan_flag = _constrain(gear, 0, 3);
	PWM_OP(fan, setDutyCycle, FAN_DUTY_LEVEL[fan_flag]);
}
void fan_control(int8_t state)
{
	int8_t next_gear = (int8_t)fan_flag + state;
	fan_set_gear(next_gear);
}

#define PPG_WAVE_COUNT 4
#define PPG_MIN_COUNT 0X3FFFF




s8   Wave_sum;  // OLED波形大小
u8   temp[6];   // 拼字节
u8   str[30];   // 字符串显示变量

//void OLED_Show(void);


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
	led_white = DO_Create(GPIOB, GPIO_Pin_12, IO_LOW);
	led_green = DO_Create(GPIOB, GPIO_Pin_13, IO_LOW);
	led_blue = DO_Create(GPIOB, GPIO_Pin_14, IO_LOW);
	led_red = DO_Create(GPIOB, GPIO_Pin_15, IO_LOW);

	fan = PWM_Create(GPIOA, GPIO_Pin_8);
	door = SG90_Create(GPIOA, GPIO_Pin_6, 0);
	fan_set_gear(0);
	AC_control(0);



	/* 模式初始化 */
	Switch_Mode(AUTO);
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
			if (Flagout == 0)
			{
				break;
			}
			delay_ms(1200);
			key_num = 0;
		}
	} while (key_num != 2);
	oled_clear_screen(0);
	oled_refresh_gram();

	/* 模块初始化 */
	SU_03T_Init(9600);

	/* 主循环 */
	while (1)
	{
		KEY_Handle();     // 按键处理
		SENSOR_Handle();  // 传感器数据处理
		AUDIO_Handle();   // 语音命令处理
		APP_Handle();     // 云平台命令处理
		//EVENT_Handle();   // 事件处理
		TIMER_Handle();   // 定时器处理

		if (MODE == MANUAL)
		{
			MANUAL_Handle();
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
	if (SU_03T_GetRxFlag())
	{
		// 优先按帧格式解析：AA 55 CMD 55 AA；不满足时回退到单字节命令
		uint8_t audio_cmd = 0;
		uint8_t rx_len = receive_data_cnt;
		if (rx_len >= 5 &&
		    receive_data_buf[0] == 0xAA &&
		    receive_data_buf[1] == 0x55 &&
		    receive_data_buf[rx_len - 2] == 0x55 &&
		    receive_data_buf[rx_len - 1] == 0xAA)
		{
			for (uint8_t i = 2; i < rx_len - 2; i++)
			{
				if (receive_data_buf[i] >= 0x01 && receive_data_buf[i] <= 0x0E)
				{
					audio_cmd = receive_data_buf[i];
					break;
				}
			}
		}
		else if (rx_len > 0)
		{
			audio_cmd = receive_data_buf[0];
		}

		switch (audio_cmd)
		{
		case 0x01:  // 打开台灯
			DO_OP(led_white, open);
			break;
		case 0x02:  // 关闭台灯
			DO_OP(led_white, close);
			break;
		case 0x03:  // 打开客厅灯
			DO_OP(led_green, open);
			break;
		case 0x04:  // 关闭客厅灯
			DO_OP(led_green, close);
			break;
		case 0x05:  // 打开风扇/风扇加速
			fan_control(1);
			break;
		case 0x06:  // 风扇减速
			fan_control(-1);
			break;
		case 0x07:  // 关闭风扇
			fan_set_gear(0);
			break;
		case 0x08:  // 空调制热
			AC_control(1);
			break;
		case 0x09:  // 空调制冷
			AC_control(2);
			break;
		case 0x0A:  // 关闭空调
			AC_control(0);
			break;
		case 0x0B:  // 打开窗户
			Door_control(1);
			break;
		case 0x0C:  // 关闭窗户
			Door_control(0);
			break;
		case 0x0D:  // 关闭所有电器
			DO_OP(led_white, close);
			DO_OP(led_green, close);
			DO_OP(led_blue, close);
			DO_OP(led_red, close);
			Door_control(0);
			AC_control(0);
			fan_set_gear(0);
			break;
		case 0x0E:  // 关闭所有灯光
			DO_OP(led_white, close);
			DO_OP(led_green, close);

			break;
		default:
			break;
		}
		SU_03T_ClearRxFlag();
	}
}

void APP_Handle()
{
	char *ptr;

	if (esp8266_cnt > 0)
	{

		/* 参数设置 */
		if ((ptr = strstr((char *)esp8266_buf, "fan_speed=")) != NULL)
		{
			uint8_t speed_cmd;
			if (sscanf(ptr, "fan_speed=%hhu", &speed_cmd) == 1)
			{
				if (speed_cmd == 1)
				{
					fan_control(1);
					SAVE_config();
					Beep_Beep(20);
				}
				else if (speed_cmd == 0)
				{
					fan_control(-1);
					SAVE_config();
					Beep_Beep(20);
				}
			}
		}


		/* 设备控制 */
		if ((ptr = strstr((char *)esp8266_buf, "led_white=")) != NULL)
		{
			int state;
			sscanf(ptr, "led_white=%d", &state);
			DO_OP(led_white, set_state, state);
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "led_green=")) != NULL)
		{
			int state;
			sscanf(ptr, "led_green=%d", &state);
			DO_OP(led_green, set_state, state);
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "led_blue=")) != NULL)
		{
			int state;
			sscanf(ptr, "led_blue=%d", &state);
			DO_OP(led_blue, set_state, state);
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "led_red=")) != NULL)
		{
			int state;
			sscanf(ptr, "led_red=%d", &state);
			DO_OP(led_red, set_state, state);
			Beep_Beep(20);
		}
		if ((ptr = strstr((char *)esp8266_buf, "door=")) != NULL)
		{
			int state;
			sscanf(ptr, "door=%d", &state);
			Door_control(state);
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


//void AUTO_Handle()
//{


	// // 单次只能处理一个事件，优先级从高到低

	// else
	// {
	// 	EVENT_SET(EVENT.cur, EVENT_NORMAL);
	// }
//}

void EVENT_Handle()
{
	// if (memcmp(&EVENT.cur, &EVENT.last, sizeof(Event_t)) != 0)
	// {
	// 	Event_t last_event;
	// 	EVENT_SET(last_event, EVENT.last.code);
	// 	EVENT_SET(EVENT.last, EVENT.cur.code);
	// 	if (Flagout == 0)
	// 	{
	// 		enum EventCode code = EVENT_NORMAL;
	// 		if (EVENT.cur.isUpload)
	// 		{
	// 			code = EVENT.cur.code;
	// 		}
	// 		sprintf(data, "cmd=2&uid=%s&topic=event&msg=$%d$event_data$", BEMFA_ID, code);
	// 		ESP8266_SendData((unsigned char *)data);
	// 	}

	// 	// 处理上一个事件的善后
	// 	switch (last_event.code)
	// 	{
		
	// 	default:
	// 		break;
	// 	}

	// 	// 处理当前事件
	// 	switch (EVENT.cur.code)
	// 	{
	// 	case EVENT_NORMAL:
	// 		DO_OP(beep, close);
	// 		break;
	
	// 	default:
	// 		break;
	// 	}
	// }
}

/***
 * 界面一
 * 时间
 * 状态
 * 天然气
 */
void Page_data1()
{
	// /* 按键处理 */
	// if (key_num == BTN_UP || key_num == BTN_DOWN)
	// {
	// 	Beep_Beep(20);
	// switch (item_idx)
	// {
	// case 0:
	// 		Switch_Mode(key_num == BTN_UP ? AUTO : MANUAL);
	// 		break;

	
	// case 1:
	// 		DO_OP(led_white, set_state, key_num == BTN_UP ? 1 : 0);
	// 		break;
	// case 2:
	// 		DO_OP(led_green, set_state, key_num == BTN_UP ? 1 : 0);
	// 		break;
	// }
	// 	SAVE_config();
	// }
	// /* 页面项目 */
	// item_idx_min = 0;
	// items_count = 0;
	

	// /* 页面显示 */
	// item_idx = _constrain(item_idx % items_count, item_idx_min, items_count - 1);
	// item_idx_start = _max(0, item_idx - item_max_count + 1);
	// for (int i = 0; i < _min(item_max_count, items_count); i++)
	// {
	// 	oled_clear_line(i);
	// 	oled_draw_text_line(i, 0, (uint8_t *)items[item_idx_start + i],
	// 	                    item_idx_start + i == item_idx ? INVERSE : NORMAL);
	// }
	// oled_refresh_gram();
	// oled_clear_screen(0);
}

/**
 * 控制界面
 * 模式
 * 阀门
 * 风扇
 * 空调
 * LED
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

	
		case 2:
				DO_OP(led_white, set_state, key_num == BTN_UP ? 1 : 0);
				break;
		case 3:
				DO_OP(led_green, set_state, key_num == BTN_UP ? 1 : 0);
				break;
		default:
			break;
		}
		SAVE_config();
	}

	/* 页面项目 */
	item_idx_min = 0;
	items_count = 0;
	const char *wifi_state = (Flagout == 0) ? "已连接" : "未连接";
	
	sprintf((char *)items[items_count++], "模式:   %s", MODE_STR[MODE]);
	sprintf((char *)items[items_count++], "WIFI状态:%s", wifi_state);
	sprintf((char *)items[items_count++], "台灯:   %s", DO_OP(led_white, is_open) ? "ON" : "OFF");
	sprintf((char *)items[items_count++], "客厅灯: %s", DO_OP(led_green, is_open) ? "ON" : "OFF");




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

* 心率上限
* 血氧下限
* 天然气上限
 */
void Page_settings()
{
	/* 按键处理 */
	if (key_num == BTN_UP || key_num == BTN_DOWN)
	{
		switch (item_idx)
		{
		case 0:
			Door_control(key_num == BTN_UP ? 1 : 0);
			break;
		case 1:
			fan_control(key_num == BTN_UP ? 1 : -1);
			break;
		case 2:
			AC_control((int8_t)ac_flag + (key_num == BTN_UP ? 1 : -1));
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

	sprintf((char *)items[items_count++], "窗户: %s", SG90_GetAngle(door) == 90 ? "ON" : "OFF");
	sprintf((char *)items[items_count++], "风扇: %s", FAN_LEVEL_STR[fan_flag]);
	sprintf((char *)items[items_count++], "空调: %s", AC_STATE_STR[ac_flag]);


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
	
}

void TIMER_Handle()
{
	if (TIMER_IT == 0)
		return;
	TIMER_IT = 0;

	

	if (Flagout == 0)
	{
		// 上传DATA数据
		// #模式#心率#血氧#天然气#阀门状态#风扇状态#空调状态#LED状态#
		sprintf(data, "cmd=2&uid=%s&topic=data&msg=", BEMFA_ID);
		_strcat_fmt(data, "#%d", MODE);
		_strcat_fmt(data, "#%s", DO_OP(led_white, is_open) ? "ON" : "OFF");
		_strcat_fmt(data, "#%s", SG90_GetAngle(door) == 90 ? "ON" : "OFF");
		_strcat_fmt(data, "#%s", DO_OP(led_green, is_open) ? "ON" : "OFF");
		_strcat_fmt(data, "#%s", DO_OP(led_blue, is_open) ? "ON" : "OFF");
		_strcat_fmt(data, "#%s", DO_OP(led_red, is_open) ? "ON" : "OFF");
		_strcat_fmt(data, "#%d", fan_flag);
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




void OLED_Show(void)
{
	
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
	DO_OP(led_white, close);
	DO_OP(led_green, close);
	DO_OP(led_blue, close);
	DO_OP(led_red, close);
	SG90_SetAngle(door, 0);
	fan_set_gear(0);
	AC_control(0);
	Door_control(0);
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