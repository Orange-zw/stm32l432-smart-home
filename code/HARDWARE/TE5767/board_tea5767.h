#ifndef _BOARD_TEA5767_H_
#define _BOARD_TEA5767_H_

/*********************************************************************
 * INCLUDES
 */
#include "stdint.h"

/*********************************************************************
 * DEFINITIONS
 */
#define TEA5767_ADDR_W 0xc0  // TEA5767 写地址
#define TEA5767_ADDR_R 0xc1  // TEA5767 读地址

#define TEA5767_MAX_KHZ 108000  // 最高频率 108M
#define TEA5767_MIN_KHZ 87500   // 最低频率 87.5M

#define TEA5767_MUTE_ON 1   // 静音
#define TEA5767_MUTE_OFF 0  // 非静音

#define TEA5767_SEARCH_UP 1    // 向上搜索
#define TEA5767_SEARCH_DOWN 0  // 向下搜索

/**
 * TEA5767 写入/读取的5字节数据结构
 * 使用 union + 匿名结构体 + 位域封装
 */
#pragma pack(push, 1)
typedef union
{
	struct
	{
		struct
		{
			uint8_t PLL_H : 6;  // bit5-0: PLL高6位
			uint8_t SM : 1;     // bit6: 搜索模式 (0=正常, 1=搜索)
			uint8_t MUTE : 1;   // bit7: 静音控制 (0=非静音, 1=静音)
		} BIT0;

		struct
		{
			uint8_t PLL_L : 8;  // bit7-0: PLL低8位
		} BIT1;

		struct
		{
			uint8_t SWP1 : 1;  // bit0: 软件可编程输出口1。若 SWP1=1 SWPOR1为高, 若 SWP1=0 SWPOR1为低
			uint8_t MR : 1;    // bit1: 若 MR=1 右声道静音强制单声道,若 MR=0 右声道非静音
			uint8_t ML : 1;    // bit2: 若 ML=1 左声道静音强制单声道,若 ML=0 左声道非静音
			uint8_t MS : 1;    // bit3: 若 MS=1 单声道,若 MS=0 立体声
			uint8_t HLSI : 1;  // bit4: 若 HLSI=1 高端本振注入,若 HLSI=0 低端本振注入
			uint8_t SSL : 2;   // bit5-6: 搜索停止电平
			uint8_t SUD : 1;   // bit7: 搜索方向 (0=向下, 1=向上)
		} BIT2;

		struct
		{
			uint8_t SI : 1;     // bit0: 若SI=1 引脚SWPORT1 作ready flag输出标志;
			uint8_t SNC : 1;    // bit1: 若 SNC =1立体声噪声消除开;
			uint8_t HCC : 1;    // bit2: 若 HCC=1高音切割开; 若 HCC=0高音切割关
			uint8_t SMUTE : 1;  // bit3: 若 SMUTE=1 软件静音开; 若 SMUTE=0 软件静音关
			uint8_t XTAL : 1;   // bit4: 若 XTAL=1 fxtal=32.768KHz; 若 XTAL=0 fxtal=13MHz
			uint8_t BL : 1;     // bit5: 若 BL=1 日本FM波段; 若 BL=0 美/欧 FM 波段
			uint8_t STBY : 1;   // bit6: 若 STBY=1 待机模式; 若 STBY=0 非待机模式
			uint8_t SWP2 : 1;   // bit7: 软件可编程输出口2。若 SWP2=1 SWPOR2为高,若 SWP2=0 SWPOR2为低
		} BIT3;

		struct
		{
			uint8_t reserve : 5;  // bit5-0: 不用管它
			uint8_t DTC : 1;      // bit6: 若 DTC=1 the 去加重时间常数为75μs; 若 DTC=0 the 去加重时间常数为50μs
			uint8_t PLLREF : 1;   // bit7: 若 PLLREF=1 则6.5 MHz 参考频率 PLL可用; 若 PLLREF=0 则6.5 MHz参考频率 PLL不可用
		} BIT4;
	};

	// 直接访问字节数组
	uint8_t data[5];
} TEA5767_data_write_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef union
{
	struct
	{
		struct
		{
			uint8_t FREQ_H : 6;  // bit5-0: 搜索或预置的电台频率值的高6位
			uint8_t BLF : 1;     // bit6: 搜索模式 (0=正常, 1=搜索)
			uint8_t RF : 1;      // bit7: 若 RF=1 发现一个电台; 若 RF=0 未发现电台
		} BIT0;

		struct
		{
			uint8_t FREQ_L : 8;  // bit7-0: 搜索或预置的电台频率值的低8位
		} BIT1;

		struct
		{
			uint8_t IF : 7;      // bit6-0: 中频计数结果
			uint8_t STEREO : 1;  // bit7: 若 STEREO=1 为立体声,若 STEREO=0 为单声道
		} BIT2;

		struct
		{
			uint8_t RESERVE : 1;  // bit0: 该位为 0;
			uint8_t CI : 3;       // bit3: 芯片标记; 设置为0
			uint8_t LEV : 4;      // bit7 - 4: 信号强度，信号电平ADC 输出
		} BIT3;

		struct
		{
			uint8_t RESERVE;  // 保留位
		} BIT4;
	};

	// 直接访问字节数组
	uint8_t data[5];
} TEA5767_data_read_t;
#pragma pack(pop)

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern uint32_t g_frequency;
extern float g_frequency_MHz;

/*********************************************************************
 * API FUNCTIONS
 */
void TEA5767_Init(void);
void TEA5767_Write(void);
void TEA5767_Read(void);
void TEA5767_GetPLL(void);
void TEA5767_SetFrequency(uint32_t frequency);
void TEA5767_SetFrequency_MHz(float fFrequency);
uint32_t TEA5767_GetFrequency(void);
uint8_t TEA5767_GetRadioLev(void);
void TEA5767_Search(uint8_t mode);
void TEA5767_AutoSearch(uint8_t mode);
void TEA5767_Mute(uint8_t mode);

#endif /* _BOARD_TEA5767_H_ */
