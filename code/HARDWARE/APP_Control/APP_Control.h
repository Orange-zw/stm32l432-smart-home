#ifndef __APP_CONTROL_H
#define __APP_CONTROL_H

#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "led.h"
#include "esp8266.h"
#include "stmflash.h"
#include <string.h>
#include <math.h>
#include "stdbool.h"

// 控制模式枚举
typedef enum {
    auto_mode,
    manual_mode
} control_mode;
// 阈值数据结构体
struct Set_Thr
{
    u16 temp_h;
    int32_t heart_rate_l;
};
// 阈值参数映射表结构体
typedef struct {
    char *cmd_name;          // 命令名称
    void *param_addr;        // 参数地址
    u8 param_type;          // 参数类型: 0-u16, 1-float
    u32 flash_addr;         // Flash存储地址
} threshold_param_t;
//报警状态变量
struct alarm
{
	u8 alarm_code;  // 0-无报警，1-温度过高，2-温度过低，3-湿度过低，
    // 4-土壤湿度过低，5-光照过低，6-PH值过高，7-PH值过低
	bool alarm_flag;  // 报警标志位
	u8 last_alarm_code;  // 记录上次的报警代码，用于单次上传控制
};
// 外部变量声明
extern unsigned char esp8266_buf[];
extern control_mode MODE;
extern struct Set_Thr Thr;
extern struct alarm alarm;
extern bool beep_flag;
extern u16 delaytimes;

// 函数声明
void APP_Control(void);                           // APP控制主函数
void Process_Threshold_Command(char *cmd_buf);    // 阈值设置通用处理函数
void Beep_Tip(int delaytimes);                   // 蜂鸣器提示函数
void ESP8266_Clear(void);                        // ESP8266清除缓存函数

#endif /* __APP_CONTROL_H */
