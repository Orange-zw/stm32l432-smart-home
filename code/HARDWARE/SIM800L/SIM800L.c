// 单片机头文件
#include "stm32f10x.h"

// 网络设备驱动
#include "SIM800L.h"

// 硬件驱动
#include "OLED.h"
#include "delay.h"
#include "ssd1306.h"

#include "esp8266.h"

// C库
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char           cmd[100];
unsigned char  SIM800L_buf[128];
unsigned short SIM800L_cnt = 0, SIM800L_cntPre = 0;
u8             SIM800L_CSQ[3];

/**
 * UTF-8 转 Unicode码位(U+ xxxx) 转换函数
 * @param unicode: 输出的 Unicode 缓冲区（UTF-16 小端序，每个字符2字节）
 * @param utf8: 输入的 UTF-8 字符串
 * @param utf8_len: UTF-8 字符串的长度
 * @return: Unicode buffer 的长度
 */
size_t UTF_8_To_Unicode(uint16_t *unicode, char *utf8, size_t utf8_len)
{
	uint8_t  *utf8_ptr = (uint8_t *)utf8;
	uint8_t  *utf8_end = utf8_ptr + utf8_len;
	uint16_t *unicode_ptr = (uint16_t *)unicode;
	size_t    unicode_count = 0;

	if (utf8 == NULL || unicode == NULL || utf8_len == 0)
	{
		return 0;
	}

	// 清空输出缓冲区（假设输出缓冲区足够大）
	// 注意：这里不清空，由调用者负责分配足够大的缓冲区

	while (utf8_ptr < utf8_end && *utf8_ptr != '\0')
	{
		uint32_t code_point = 0;
		uint8_t  byte1 = utf8_ptr[0];
		size_t   bytes_consumed = 0;

		// 1字节字符 (ASCII: 0x00-0x7F)
		if ((byte1 & 0x80) == 0)
		{
			code_point = byte1;
			bytes_consumed = 1;
		}
		// 2字节字符 (0x80-0x7FF)
		else if ((byte1 & 0xE0) == 0xC0)
		{
			if (utf8_ptr + 2 > utf8_end)
				break;
			// 检查后续字节是否为有效的UTF-8继续字节
			if ((utf8_ptr[1] & 0xC0) != 0x80)
				break;
			code_point = ((byte1 & 0x1F) << 6) | (utf8_ptr[1] & 0x3F);
			bytes_consumed = 2;
		}
		// 3字节字符 (0x800-0xFFFF) - 支持大部分中文
		else if ((byte1 & 0xF0) == 0xE0)
		{
			if (utf8_ptr + 3 > utf8_end)
				break;
			// 检查后续字节是否为有效的UTF-8继续字节
			if ((utf8_ptr[1] & 0xC0) != 0x80 || (utf8_ptr[2] & 0xC0) != 0x80)
				break;
			code_point = ((byte1 & 0x0F) << 12) | ((utf8_ptr[1] & 0x3F) << 6) |
			             (utf8_ptr[2] & 0x3F);
			bytes_consumed = 3;
		}
		// 4字节字符 (0x10000-0x10FFFF) - 超出BMP，需要代理对
		// 嵌入式系统通常不支持，这里简化为跳过或使用替换字符
		else if ((byte1 & 0xF8) == 0xF0)
		{
			if (utf8_ptr + 4 > utf8_end)
				break;
			// 检查后续字节是否为有效的UTF-8继续字节
			if ((utf8_ptr[1] & 0xC0) != 0x80 || (utf8_ptr[2] & 0xC0) != 0x80 ||
			    (utf8_ptr[3] & 0xC0) != 0x80)
				break;
			// 超出BMP的字符，使用替换字符 0xFFFD
			code_point = 0xFFFD;  // Unicode 替换字符
			bytes_consumed = 4;
		}
		else
		{
			// 无效的UTF-8序列，跳过当前字节
			utf8_ptr += 1;
			continue;
		}

		// 将码点转换为UTF-16（小端序）
		// 对于BMP字符（0x0000-0xFFFF），直接使用
		if (code_point <= 0xFFFF)
		{
			unicode_ptr[unicode_count++] = (uint16_t)code_point;
		}
		// 超出BMP的字符需要代理对，这里简化为替换字符
		else
		{
			unicode_ptr[unicode_count++] = 0xFFFD;  // Unicode 替换字符
		}

		// 移动UTF-8指针
		utf8_ptr += bytes_consumed;
	}

	// 确保字符串以null结尾（2字节的null）
	unicode_ptr[unicode_count] = 0;

	return unicode_count;  // 返回Unicode字符数量
}

size_t UTF_8_To_Unicode_String(char *str, char *utf8, size_t utf8_len)
{
	size_t   pos = 0;
	uint16_t unicode[100];
	size_t   unicode_len = UTF_8_To_Unicode(unicode, utf8, utf8_len);
	for (int i = 0; i < unicode_len; i++)
	{
		sprintf(str + pos, "%04X", (unsigned)unicode[i]);
		pos += 4;
	}
	str[pos] = '\0';
	return pos;
}

void SIM800L_Init(unsigned int bound)
{
#if SIM800L_USART1 == 1
	// GPIO端口设置
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA,
	                       ENABLE);  // 使能USART1，GPIOA时钟

	// USART1_TX   GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;  // PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);           // 初始化GPIOA.9

	// USART1_RX	  GPIOA.10初始化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;             // PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);                 // 初始化GPIOA.10

	// Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;  // 抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;         // 子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);                            // 根据指定的参数初始化VIC寄存器

	// USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;  // 串口波特率
	USART_InitStructure.USART_WordLength =
	    USART_WordLength_8b;                                // 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;  // 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;     // 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl =
	    USART_HardwareFlowControl_None;                              // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  // 收发模式

	USART_Init(USART1, &USART_InitStructure);       // 初始化串口1
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  // 开启串口接受中断
	USART_Cmd(USART1, ENABLE);                      // 使能串口1

#elif SIM800L_USART2 == 1
	GPIO_InitTypeDef  gpio_initstruct;
	USART_InitTypeDef usart_initstruct;
	NVIC_InitTypeDef  nvic_initstruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	// PA2	TXD
	gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_2;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_initstruct);

	// PA3	RXD
	gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio_initstruct.GPIO_Pin = GPIO_Pin_3;
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_initstruct);

	usart_initstruct.USART_BaudRate = bound;
	usart_initstruct.USART_HardwareFlowControl =
	    USART_HardwareFlowControl_None;                           // 无硬件流控
	usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  // 接收和发送
	usart_initstruct.USART_Parity = USART_Parity_No;              // 无校验
	usart_initstruct.USART_StopBits = USART_StopBits_1;           // 1位停止位
	usart_initstruct.USART_WordLength = USART_WordLength_8b;      // 8位数据位
	USART_Init(USART2, &usart_initstruct);

	USART_Cmd(USART2, ENABLE);  // 使能串口

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  // 使能接收中断

	nvic_initstruct.NVIC_IRQChannel = USART2_IRQn;
	nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
	nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 1;
	nvic_initstruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic_initstruct);

#elif SIM800L_USART3 == 1
	NVIC_InitTypeDef  NVIC_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	USART_DeInit(USART3);  // 复位串
	// USART2_TX   PB10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;  // PB10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽输出
	GPIO_Init(GPIOB, &GPIO_InitStructure);           // 初始化PB10

	// USART3_RX	  PB11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 浮空输入
	GPIO_Init(GPIOB, &GPIO_InitStructure);                 // 初始化PB11

	USART_InitStructure.USART_BaudRate = bound;  // 一般设置为9600;
	USART_InitStructure.USART_WordLength =
	    USART_WordLength_8b;                                // 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;  // 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;     // 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl =
	    USART_HardwareFlowControl_None;                              // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  // 收发模式

	USART_Init(USART3, &USART_InitStructure);  // 初始化串口	3

	USART_Cmd(USART3, ENABLE);  // 使能串口
	// 使能接收中断
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);  // 开启中断

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  // 抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;         // 子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);                            // 根据指定的参数初始化VIC寄存器
#endif
}

//==========================================================
//	函数名称：	SIM800L_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void SIM800L_Clear(void)
{

	memset(SIM800L_buf, 0, sizeof(SIM800L_buf));
	SIM800L_cnt = 0;
}

//==========================================================
//	函数名称：	SIM800L_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool SIM800L_WaitRecive(void)
{

	if (SIM800L_cnt ==
	    0)  // 如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return 1;

	if (SIM800L_cnt == SIM800L_cntPre)  // 如果上一次的值和这次相同，则说明接收完毕
	{
		SIM800L_cnt = 0;  // 清0接收计数

		return 0;  // 返回接收完成标志
	}

	SIM800L_cntPre = SIM800L_cnt;  // 置为相同

	return 1;  // 返回接收未完成标志
}

//==========================================================
//	函数名称：	SIM800L_SendCmd
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
_Bool SIM800L_SendCmd(char *cmd, char *res)
{

	unsigned char timeOut = 200;

	Usart_SendString(USART3, (unsigned char *)cmd, strlen((const char *)cmd));

	while (timeOut--)
	{
		if (SIM800L_WaitRecive() == 0)  // 如果收到数据
		{
			if (strstr((const char *)SIM800L_buf, res) != NULL)  // 如果检索到关键词
			{
				SIM800L_Clear();  // 清空缓存

				return 0;
			}
		}

		delay_ms(10);
	}

	return 1;
}

void SIM800L_Send_En_message(char *number, char *content)  // 发送英文短信
{
	char cmd[100];

	//  LCD_ShowString(43,  128,"AT+CMGF=1",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CMGF=1\r\n", "OK"))
		delay_ms(1000);
	//  LCD_ShowString(43,  128,"AT+CSCS",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CSCS=\"GSM\"\r\n", "OK"))
		delay_ms(1000);
	//  LCD_ShowString(43,  128,"AT+CSCA?",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CSCA?\r\n", "OK"))
		delay_ms(1000);
	//  LCD_ShowString(43,  128,"AT+CSMP",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CSMP=17,167,0,241\r\n", "OK"))
		delay_ms(1000);
	//  LCD_ShowString(43,  128,"AT+CMGS=",RED,WHITE,16,0);
	sprintf((char *)cmd, "AT+CMGS=\"%s\"\r\n", number);
	while (SIM800L_SendCmd(cmd, ">"))
		delay_ms(1000);
	Usart_SendString(USART3, (u8 *)content, strlen(content));
	//  LCD_ShowString(43,  128,"0x1A",RED,WHITE,16,0);
	USART3->DR = (u32)0x1A;
	delay_ms(1000);

	//  LCD_ShowChinese(43,  128,"发送成功！",RED,WHITE,16,0);
}

void SIM800L_Send_Cn_message(char *number, char *content)  // 发送中文短信
{

	memset(cmd, 0, sizeof(cmd));
	SIM800L_Clear();  // 清空缓存
	//    LCD_ShowChinese(43,  128,"发送短信中",RED,WHITE,16,0);
	//    LCD_ShowString(43,  146,"10",RED,WHITE,16,0);
	// OLED_ShowString(76, 2, (u8 *)"1", 16);
	oled_clear_screen(0);
	oled_draw_text_line(0, 0, (uint8_t *)"CN消息发送中:", NORMAL);
	oled_draw_text_line(1, 76, (uint8_t *)"10%", NORMAL);
	oled_refresh_gram();
	while (SIM800L_SendCmd("AT\r\n", "OK"))
		delay_ms(1000);
	oled_draw_text_line(1, 76, (uint8_t *)"20%", NORMAL);
	oled_refresh_gram();
	while (SIM800L_SendCmd("AT&F\r\n", "OK"))
		delay_ms(1000);
	oled_draw_text_line(1, 76, (uint8_t *)"30%", NORMAL);
	oled_refresh_gram();
	//    LCD_ShowString(43,  146,"30",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CMGF=1\r\n", "OK"))
		delay_ms(1000);
	oled_draw_text_line(1, 76, (uint8_t *)"40%", NORMAL);
	oled_refresh_gram();
	//    LCD_ShowString(43,  146,"76",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CSCS=\"UCS2\"\r\n", "OK"))
		delay_ms(1000);
	oled_draw_text_line(1, 76, (uint8_t *)"50%", NORMAL);
	oled_refresh_gram();
	//    LCD_ShowString(43,  146,"50",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CSMP=17,167,0,8\r\n", "OK"))
		delay_ms(1000);
	// OLED_ShowString(76, 2, (u8 *)"6", 16);
	//    LCD_ShowString(43,  146,"60",RED,WHITE,16,0);
	oled_draw_text_line(1, 76, (uint8_t *)"60%", NORMAL);
	oled_refresh_gram();

	char   number_unicode[256];
	size_t number_unicode_len =
	    UTF_8_To_Unicode_String(number_unicode, number, strlen(number));

	sprintf((char *)cmd, "AT+CMGS=\"%s\"\r\n", number_unicode);
	while (SIM800L_SendCmd(cmd, ">"))
		delay_ms(1000);

	char   content_unicode[256];
	size_t content_unicode_len =
	    UTF_8_To_Unicode_String(content_unicode, content, strlen(content));

	Usart_SendString(USART3, (unsigned char *)content_unicode, content_unicode_len);
	USART3->DR = (u32)0x1A;
	delay_ms(1000);
	memset(cmd, 0, sizeof(cmd));
	SIM800L_Clear();
	// OLED_ShowString(76, 2, (u8 *)"succ", 16);
	oled_draw_text_line(1, 76, (uint8_t *)"100%", NORMAL);
	oled_refresh_gram();
	// 清空缓存
	//    LCD_ShowChinese(43,  146,"发送成功！",RED,WHITE,16,0);
}

void SIM800L_Make_Call(char *number)  // 拨打电话
{
	char cmd[20];

	//  LCD_ShowString(43,  128,"AT",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT\r\n", "OK"))
		delay_ms(1000);

	//  LCD_ShowString(43,  128,"AT+CPIN?",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CPIN?\r\n", "READY"))  // 没有SIM卡
		delay_ms(1000);

	//  LCD_ShowString(43,  128,"AT+CREG?",RED,WHITE,16,0);
	while (SIM800L_SendCmd("AT+CREG?\r\n", "0,1"))
	{
		while (strstr((const char *)SIM800L_buf, "0,5") == NULL)
		{
			//       LCD_ShowString(43,  128,"AT+CSQ",RED,WHITE,16,0);
			while (!SIM800L_SendCmd("AT+CSQ\r\n", "OK"))
			{
				memcpy(SIM800L_CSQ, SIM800L_buf + 15, 2);
			}
			return;  // 等待附着到网络
		}
	}

	//  LCD_ShowChinese(43,  128,"拨打中！",RED,WHITE,16,0);
	sprintf(cmd, "ATD%s;\r\n", number);
	while (SIM800L_SendCmd(cmd, "OK"))
		delay_ms(1000);

	//  LCD_ShowChinese(43,  128,"拨打成功！",RED,WHITE,16,0);
}

//==========================================================
//	函数名称：	USART3_IRQHandler
//
//	函数功能：	串口收发中断
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================

#if SIM800L_USART1
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  // 接收中断
	{
		if (SIM800L_cnt >= sizeof(SIM800L_buf))
			SIM800L_cnt = 0;  // 防止串口被刷爆
		SIM800L_buf[SIM800L_cnt++] = USART1->DR;
		USART_ClearFlag(USART1, USART_FLAG_RXNE);
	}
}

#elif SIM800L_USART2
void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  // 接收中断
	{
		if (SIM800L_cnt >= sizeof(SIM800L_buf))
			SIM800L_cnt = 0;  // 防止串口被刷爆
		SIM800L_buf[SIM800L_cnt++] = USART2->DR;
		USART_ClearFlag(USART2, USART_FLAG_RXNE);
	}
}

#elif SIM800L_USART3
void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  // 接收中断
	{
		if (SIM800L_cnt >= sizeof(SIM800L_buf))
			SIM800L_cnt = 0;  // 防止串口被刷爆
		SIM800L_buf[SIM800L_cnt++] = USART3->DR;
		USART_ClearFlag(USART3, USART_FLAG_RXNE);
	}
}

#endif

/*
************************************************************
*	函数名称：	Usart_SendString
*
*	函数功能：	串口数据发送
*
*	入口参数：	USARTx：串口组
*				str：要发送的数据
*				len：数据长度
*
*	返回参数：	无
*
*	说明：
************************************************************
*/
// void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned
// short len)
//{
//	unsigned short count = 0;
//
//	for(; count < len; count++)
//	{
//		USART_SendData(USARTx, *str++);
////发送数据 		while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) ==
/// RESET); /等待发送完成
//	}

//}
