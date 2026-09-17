#include "APP_Control.h"

// 阈值参数映射表
static const threshold_param_t threshold_params[] = {
    {"SET_temp_h",      &Thr.temp_h,       0, FLASH_SAVE_ADDR+0},
    {"SET_heart_rate_l",      &Thr.heart_rate_l,       0, FLASH_SAVE_ADDR+2}

};

#define THRESHOLD_PARAM_COUNT (sizeof(threshold_params)/sizeof(threshold_param_t))
	
//APP发送消息控制函数,每次收到消息后蜂鸣器短暂提示
// 阈值设置通用处理函数
void Process_Threshold_Command(char *cmd_buf)
{
    u8 i;
    char *ptr;
    
    // 遍历参数映射表
    for(i = 0; i < THRESHOLD_PARAM_COUNT; i++)
    {
        if(strstr(cmd_buf, threshold_params[i].cmd_name) != 0)
        {
            ptr = strstr(cmd_buf, threshold_params[i].cmd_name);
            if(ptr != NULL)
            {
                // 跳过命令名称
                ptr += strlen(threshold_params[i].cmd_name);
                
                if(threshold_params[i].param_type == 0) // u16类型
                {
                    u16 value = 0;
                    // 解析数值直到遇到'#'
                    while(*ptr != '#' && *ptr != '\0')
                    {
                        if(*ptr >= '0' && *ptr <= '9')
                        {
                            value = value * 10 + (*ptr - '0');
                        }
                        ptr++;
                    }
                    if(*ptr == '#')
                    {
                        *(u16*)threshold_params[i].param_addr = value;
                        STMFLASH_Write(threshold_params[i].flash_addr, (u16*)threshold_params[i].param_addr, 1);
                        Beep_Tip(delaytimes);
                    }
                }
                else if(threshold_params[i].param_type == 1) // float类型
                {
                    float value = 0.0;
                    u8 decimal_places = 0;
                    u8 is_decimal = 0;
                    
                    // 解析浮点数直到遇到'#'
                    while(*ptr != '#' && *ptr != '\0')
                    {
                        if(*ptr >= '0' && *ptr <= '9')
                        {
                            if(is_decimal)
                            {
                                decimal_places++;
                                value = value + (*ptr - '0') / pow(10, decimal_places);
                            }
                            else
                            {
                                value = value * 10 + (*ptr - '0');
                            }
                        }
                        else if(*ptr == '.' && !is_decimal)
                        {
                            is_decimal = 1;
                        }
                        ptr++;
                    }
                    if(*ptr == '#')
                    {
                        *(float*)threshold_params[i].param_addr = value;
                        STMFLASH_Write(threshold_params[i].flash_addr, (u16*)threshold_params[i].param_addr, 2);
                        Beep_Tip(delaytimes);
                    }
                }
                break; // 找到匹配的命令后退出循环
            }
        }
    }
}

// APP控制主函数
void APP_Control(void)
{
     //阈值设置命令解析 - 通用处理函数
    if(strstr((const char *)esp8266_buf,"SET_")!=0)
    {
        Process_Threshold_Command((char *)esp8266_buf);
    }

    ESP8266_Clear();
}

