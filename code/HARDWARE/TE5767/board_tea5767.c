/*********************************************************************
 * INCLUDES
 */
#include "stdio.h"
#include "stdlib.h"

#include <stdint.h>
#include "board_i2c.h"
#include "board_tea5767.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint32_t g_frequency = TEA5767_MIN_KHZ;
float g_frequency_MHz = 87.5;

/*********************************************************************
 * LOCAL VARIABLES
 */
static TEA5767_data_write_t s_radioWriteData = {.data = {0x31, 0xA0, 0x20, 0x11, 0x00}};
static TEA5767_data_read_t s_radioReadData = {.data = {0}};
static uint32_t s_pll = 0;

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 @brief 初始化TEA5767
 @param 无
 @return 无
*/
void TEA5767_Init(void)
{
	IIC_Init();
	TEA5767_SetFrequency_MHz(87.5);
	g_frequency = TEA5767_GetFrequency();
}

/**
 @brief 向TEA5767写入5个字节数据
 @param 无
 @return 无
*/
void TEA5767_Write(void)
{
	uint8_t i;

	IIC_Start();                   // 发送起始信号
	IIC_SendByte(TEA5767_ADDR_W);  // TEA5767写地址
	IIC_WaitAck();                 // 等待应答
	for (i = 0; i < 5; i++)
	{
		IIC_SendByte(s_radioWriteData.data[i]);  // 连续写入5个字节数据
		IIC_Ack();                               // 发送应答
	}
	IIC_Stop();  // 发送停止信号
}

/**
 @brief 读TEA5767状态
 @param 无
 @return 无
*/
void TEA5767_Read(void)
{
	uint8_t i;
	uint8_t tempLow;
	uint8_t tempHigh;
	s_pll = 0;

	IIC_Start();
	IIC_SendByte(TEA5767_ADDR_R);  // TEA5767读地址
	IIC_WaitAck();
	for (i = 0; i < 5; i++)  // 读取5个字节数据
	{
		s_radioReadData.data[i] = IIC_ReadByte();  // 读取数据后，发送应答
	}
	IIC_Stop();
	tempLow = s_radioReadData.data[1];   // 得到s_pll低8位
	tempHigh = s_radioReadData.data[0];  // 得到s_pll高6位
	tempHigh &= 0x3f;
	s_pll = tempHigh * 256 + tempLow;  // PLL值
}

/**
 @brief 由频率计算PLL
 @param 无
 @return 无
*/
void TEA5767_GetPLL(void)
{
	uint8_t hlsi;
	hlsi = s_radioWriteData.data[2] & 0x10;  // HLSI位
	if (hlsi)
	{
		s_pll = (uint32_t)((float)((g_frequency + 225) * 4) / (float)32.768);  // 频率单位:k
	}
	else
	{
		s_pll = (uint32_t)((float)((g_frequency - 225) * 4) / (float)32.768);  // 频率单位:k
	}
}

/**
 @brief 设置频率
 @param frequency -[in] 频率，单位:KHz
 @return 无
*/
void TEA5767_SetFrequency(uint32_t frequency)
{
	g_frequency = frequency;
	TEA5767_GetPLL();
	s_radioWriteData.data[0] = s_pll / 256;
	s_radioWriteData.data[1] = s_pll % 256;
	s_radioWriteData.data[2] = 0x20;
	s_radioWriteData.data[3] = 0x11;
	s_radioWriteData.data[4] = 0x00;

	TEA5767_Write();
}

/**
 @brief 设置频率
 @param frequency -[in] 频率，单位:MHz
 @return 无
*/
void TEA5767_SetFrequency_MHz(float fFrequency)
{
	g_frequency_MHz = fFrequency;
	g_frequency = fFrequency * 1000;
    
	unsigned int nPLL;
	nPLL = (unsigned int)((fFrequency * 1000000 + 225000) / 32768 * 4);
	s_radioWriteData.data[0] = (unsigned char)(nPLL >> 8);
	s_radioWriteData.data[1] = (unsigned char)(nPLL & 0xff);
	// s_radioWriteData.BIT0.PLL_H = nPLL >> 8;
	// s_radioWriteData.BIT1.PLL_L = nPLL & 0xff;
	s_radioWriteData.data[2] = 0xb0;
	s_radioWriteData.data[3] = 0x10;
	s_radioWriteData.data[4] = 0x0;

	TEA5767_Write();
}

/**
 @brief 由PLL计算频率
 @param 无
 @return 无
*/
uint32_t TEA5767_GetFrequency(void)
{
	// uint8_t hlsi;
	// uint32_t pll = 0;
	// pll = s_pll;
	// hlsi = s_radioWriteData.data[2] & 0x10;
	// if (hlsi)
	// {
	// 	g_frequency = (unsigned long)((float)(pll) * (float)8.192 - 225);  // 频率单位:KHz
	// }
	// else
	// {
	// 	g_frequency = (unsigned long)((float)(pll) * (float)8.192 + 225);  // 频率单位:KHz
	// }
	// return g_frequency;
	TEA5767_Read();
	uint32_t frequency = 0;
	frequency = s_radioReadData.BIT0.FREQ_H * 256 + s_radioReadData.BIT1.FREQ_L;
	frequency = (frequency - 225) * 8.192;
	return frequency;
}
/**
 @brief 获取信号强度
 @param 无
 @return 信号强度 0-15 越大信号越强
*/
uint8_t TEA5767_GetRadioLev(void)
{

	TEA5767_Read();
	return s_radioReadData.BIT3.LEV;
}

/**
 @brief 手动搜索电台，不用考虑TEA5767用于搜台的相关位:SM,SUD
 @param mode -[in] 搜索方式，mode=1，向上搜索，频率值+0.1MHz；mode=0，向下搜索，频率值-0.1MHz
 @return 无
*/
void TEA5767_Search(uint8_t mode)
{
	TEA5767_Read();  // 读取当前频率值
	if (mode)        // 向上搜索
	{
		g_frequency += 100;
		if (g_frequency > TEA5767_MAX_KHZ)  // 频率达到最大值
		{
			g_frequency = TEA5767_MIN_KHZ;
		}
	}
	else
	{
		g_frequency -= 100;
		if (g_frequency < TEA5767_MIN_KHZ)
		{
			g_frequency = TEA5767_MAX_KHZ;
		}
	}
	TEA5767_GetPLL();  // 计算PLL值
	s_radioWriteData.data[0] = s_pll / 256;
	s_radioWriteData.data[1] = s_pll % 256;
	s_radioWriteData.data[2] = 0x20;
	s_radioWriteData.data[3] = 0x11;
	s_radioWriteData.data[4] = 0x00;
	TEA5767_Write();
	TEA5767_Read();
	if (s_radioReadData.data[0] & 0x80)  // 搜台成功，RF=1，可保存其频率值待用
	{
		printf(" frequency=%u\n", g_frequency);
	}
	return;
}

/**
 @brief 自动搜索电台
 @param mode -[in] 搜索方式，mode=1，频率增加搜台，频率值+0.1MHz；mode=0，频率减小搜台
 @return 无
*/
void TEA5767_AutoSearch(uint8_t mode)
{
	uint8_t radioRf = 0;   // 1=发现一个电台，0=未找到电台
	uint8_t radioIf = 0;   // 中频计数结果
	uint8_t radioLev = 0;  // 信号电平ADC输出

	// 直到搜台成功，RF=1,0x31<IF<0x3E
	while ((radioRf == 0) || ((0x31 >= radioIf) || (radioIf >= 0x3E)))
	// while ((radioRf == 0) || (radioIf <= 0x31) || (radioIf >= 0x3E))
	{
		if (mode)  // 频率增加搜台
		{
			s_radioWriteData.data[2] = 0xC0;  // SUD=1,SSLadc=7,HLSI=0
			g_frequency += 100;
			if (g_frequency > TEA5767_MAX_KHZ)  // 频率达到最大值
			{
				g_frequency = TEA5767_MIN_KHZ;
			}
		}
		else  // 频率减小搜台
		{
			s_radioWriteData.data[2] = 0x40;  // SUD=0,SSLadc=7,HLSI=0
			g_frequency -= 100;
			if (g_frequency < TEA5767_MIN_KHZ)
			{
				g_frequency = TEA5767_MAX_KHZ;
			}
		}
		TEA5767_GetPLL();                               // 转换为PLL值
		s_radioWriteData.data[0] = s_pll / 256 + 0xC0;  // MUTE=1,SM=1
		s_radioWriteData.data[1] = s_pll % 256;
		s_radioWriteData.data[3] = 0x11;
		s_radioWriteData.data[4] = 0x00;
		TEA5767_Write();  // 写入5个字节数据
		TEA5767_Read();   // 读取当前频率值
		radioRf = s_radioReadData.data[0] & 0x80;
		radioIf = s_radioReadData.data[2] & 0x7F;
		radioLev = s_radioReadData.data[3] >> 4;
		printf(" r=%02x\n", radioRf);
		printf(" i=%02x\n", radioIf);
		printf(" l=%d\n", radioLev);
		printf(" f=%d\n", (int)g_frequency);
	}
	TEA5767_GetPLL();                        // 转换为PLL值
	s_radioWriteData.data[0] = s_pll / 256;  // MUTE=0,SM=0
	s_radioWriteData.data[1] = s_pll % 256;
	s_radioWriteData.data[3] = 0x11;
	s_radioWriteData.data[4] = 0x00;
	TEA5767_Write();  // 写入5个字节数据
	TEA5767_Read();   // 读取当前频率值
	printf(" frequency=%d\n", (int)g_frequency);
}

/**
 @brief 静音
 @param mode -[in] 静音方式，mode=1，静音；mode=0，非静音
 @return 无
*/
void TEA5767_Mute(uint8_t mode)
{
	s_radioWriteData.BIT0.MUTE = mode;

	TEA5767_Write();
}

/****************************************************END OF FILE****************************************************/
