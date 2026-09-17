// 单片机头文件
#include "lcd.h"
#include "stm32f10x.h"

// 网络设备驱动
#include "M5310A.h"

// 硬件驱动
#include "GUI.h"
#include "delay.h"
#include "usart_driver.h"

// C库
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 单例模式：全局唯一的M5310A实例
static USART_handle_t m5310a_instance = NULL;

// 接收缓冲区
unsigned char M5310A_buf[M5310A_BUF_SIZE];
static uint16_t M5310A_rx_len = 0;

// 外部状态
extern char Flagout;

// 内部变量
static char count = 0;
static char m5310a_str[50];

// 内部函数声明

static void M5310A_rx_callback(uint8_t *data, uint16_t length)
{

	memcpy(M5310A_buf, data, length);
	M5310A_rx_len = length;
}

//==========================================================
//	函数名称：	M5310A_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void M5310A_Clear(void)
{
	if (m5310a_instance != NULL)
	{
		USART_OP(m5310a_instance, flush_rx);
	}
	memset(M5310A_buf, 0, sizeof(M5310A_buf));
	M5310A_rx_len = 0;
}

//==========================================================
//	函数名称：	M5310A_RxReady
//
//	函数功能：	检查是否有接收数据，并自动更新接收缓冲区
//
//	入口参数：	无
//
//	返回参数：	1-有数据，0-无数据
//
//	说明：		如果检测到有数据，会自动更新M5310A_buf
//==========================================================
_Bool M5310A_RxReady(void)
{
	if (m5310a_instance == NULL)
		return 0;

	return M5310A_rx_len > 0;
}

void M5310A_GetRxData(unsigned char *data)
{
	if (m5310a_instance == NULL)
		return;

	memcpy(data, M5310A_buf, M5310A_rx_len);
	M5310A_rx_len = 0;
}

//==========================================================
//	函数名称：	M5310A_Init
//
//	函数功能：	初始化M5310A
//
//	入口参数：	instance - USART实例（USART1/USART2/USART3）
//
//	返回参数：	无
//
//	说明：		如果已经初始化过，则直接返回
//==========================================================
void M5310A_Init(USART_TypeDef *instance)
{
	// 单例模式：如果已经初始化，直接返回
	if (m5310a_instance != NULL)
	{
		return;
	}

	// 创建USART实例
	m5310a_instance = USART_Create(instance, 9600);
	if (m5310a_instance == NULL)
	{
		return;  // 创建失败
	}

	USART_OP(m5310a_instance, set_rx_idle_callback, M5310A_rx_callback);

	// 初始化复位引脚
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(M5310A_RST_RCC_CLK, ENABLE);  // 使能A端口时钟

	GPIO_InitStructure.GPIO_Pin = M5310A_RST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   // 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 速度50MHz
	GPIO_Init(M5310A_RST_PROT, &GPIO_InitStructure);   // 初始化GPIOA

	GPIO_SetBits(M5310A_RST_PROT, M5310A_RST_PIN);
	delay_ms(500);
	GPIO_ResetBits(M5310A_RST_PROT, M5310A_RST_PIN);  // M5310A低电平复位

	M5310A_Clear();

	LCD_Clear(0);

	_lcd_dev temp;
	memcpy(&temp, &lcddev, sizeof(_lcd_dev));

	Show_Str(32, 48, YELLOW, BLACK, (u8 *)"正在连网...", 16, 0);
	Show_Str(32, 64, YELLOW, BLACK, (u8 *)"进度:.", 16, 0);
	Show_Str(76, 64, YELLOW, BLACK, (u8 *)"10%", 16, 0);

START:
	// 测试M5310-A是否开机正常
	count = 0;
	Show_Str(76, 64, YELLOW, BLACK, (u8 *)"10%", 16, 0);
	while (M5310A_SendCmd("AT\r\n", "OK"))
		delay_ms(1000);

	// 重复检查是否正常驻网，正确驻网时间一般为开机后10秒左右，驻网成功才有网络，才可以进行下一步
	Show_Str(76, 64, YELLOW, BLACK, (u8 *)"20%", 16, 0);
	while (M5310A_SendCmd("AT+CGATT?\r\n", "+CGATT:1"))
		delay_ms(1000);

	// 创建TCP Socket连接，开启自动上报接收数据
	Show_Str(76, 64, YELLOW, BLACK, (u8 *)"30%", 16, 0);
	while (M5310A_SendCmd("AT+NSOCR=\"STREAM\",6,0,2\r\n", "OK") && Flagout)
		delay_ms(1000);

	// 连接TCP远程服务器
	Show_Str(76, 64, YELLOW, BLACK, (u8 *)"50%", 16, 0);
	while (M5310A_SendCmd("AT+NSOCO=1,bemfa.com ,8344\r\n", "CONNECT OK") && Flagout)
	{
		delay_ms(1000);
		if (count++ > 10) goto START;
	}

	// 配置收发模式
	Show_Str(76, 64, YELLOW, BLACK, (u8 *)"60%", 16, 0);
	while (M5310A_SendCmd("AT+NSOCFG=1,0,0\r\n", "OK") && Flagout)
		delay_ms(1000);

	// 发送订阅指令
	Show_Str(76, 64, YELLOW, BLACK, (u8 *)"80%", 16, 0);
	sprintf(m5310a_str, "AT+NSOSD=1,1,\"cmd=1&uid=%s&topic=%s\\r\\n\",,4\r\n", BEMFA_ID, CONTROL_TOPIC);
	while (M5310A_SendCmd(m5310a_str, "cmd=1&res=1") && Flagout)
		delay_ms(1000);

	memcpy(&lcddev, &temp, sizeof(lcddev));
	Flagout = 3;
	Show_Str(76, 64, YELLOW, BLACK, (u8 *)"100%", 16, 0);
	delay_ms(1000);
	Flagout = 0;
	delay_ms(2000);
	M5310A_Clear();
}

//==========================================================
//	函数名称：	M5310A_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：
//==========================================================
_Bool M5310A_SendCmd(char *cmd, char *res)
{
	if (m5310a_instance == NULL)
		return 1;

	unsigned char timeOut = 200;
	uint8_t rx_data[256];
	// 使用USART驱动发送数据
	USART_OP(m5310a_instance, send, (uint8_t *)cmd, strlen((const char *)cmd));

	while (timeOut--)
	{
		// 更新接收缓冲区
		if (M5310A_RxReady())  // 如果收到数据
		{
			M5310A_GetRxData(rx_data);
			if (strstr((const char *)rx_data, res) != NULL)  // 如果检索到关键词
			{
				M5310A_Clear();  // 清空缓存
				return 0;
			}
			else
			{
				return 1;  // 失败
			}
		}

		delay_ms(10);
	}

	return 1;
}

//==========================================================
//	函数名称：	M5310A_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	uid：用户ID（未使用，保留兼容性）
//				topic：主题
//				msg：消息
//
//	返回参数：	无
//
//	说明：
//==========================================================
void M5310A_SendData(unsigned char *uid, unsigned char *topic, unsigned char *msg)
{
	if (m5310a_instance == NULL)
		return;

	char buf_str[256];
	sprintf(buf_str, "AT+NSOSD=1,1,\"cmd=2&uid=%s&topic=%s&msg=%s\\r\\n\",,4\r\n", BEMFA_ID, topic, msg);
	while (M5310A_SendCmd(buf_str, "cmd=2&res=1"))
	{
		delay_ms(100);
	}
}
