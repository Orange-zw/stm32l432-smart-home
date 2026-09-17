/* ============================================================
 * 模块     : 4x4 矩阵按键驱动
 * 功能     : 矩阵键盘扫描与按键值解析
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "button4_4.h"
#include <stdbool.h>

struct IO_PORT
{
	GPIO_TypeDef *GPIO_x;
	unsigned short GPIO_pin;
};
static struct IO_PORT KEY_OUT[4] = {
    {BUTTON_ROW1_GPIO_PORT, BUTTON_ROW1_GPIO_PIN},
    {BUTTON_ROW2_GPIO_PORT, BUTTON_ROW2_GPIO_PIN},
    {BUTTON_ROW3_GPIO_PORT, BUTTON_ROW3_GPIO_PIN},
    {BUTTON_ROW4_GPIO_PORT, BUTTON_ROW4_GPIO_PIN}};
static struct IO_PORT KEY_IN[4] = {
    {BUTTON_COL1_GPIO_PORT, BUTTON_COL1_GPIO_PIN},
    {BUTTON_COL2_GPIO_PORT, BUTTON_COL2_GPIO_PIN},
    {BUTTON_COL3_GPIO_PORT, BUTTON_COL3_GPIO_PIN},
    {BUTTON_COL4_GPIO_PORT, BUTTON_COL4_GPIO_PIN}};
unsigned char key[4][4];

// 键盘映射表（默认布局）
// 需要根据实际按键布局修改
const uint8_t default_key_map[4][4] = {
    {1, 4, 7, '*'},        // 第1行
    {2, 5, 8, 0},          // 第2行
    {3, 6, 9, '#'},        // 第3行
    {'A', 'B', 'C', '\n'}  // 第4行
};

// 实际使用的键盘映射表（根据旋转和镜像参数从default_key_map生成）
uint8_t key_map[4][4];

// 根据旋转角度和镜像生成key_map
static void GenerateKeyMap(rotation_t rotation, bool mirror)
{
	uint8_t temp_map[4][4];
	uint8_t i, j;
	uint8_t src_i, src_j;

	// 先应用旋转
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			// 根据旋转角度计算源位置
			// 旋转是顺时针方向，从default_key_map读取数据
			switch (rotation)
			{
			case ROTATION_0:  // 不旋转: (i,j) -> (i,j)
				src_i = i;
				src_j = j;
				break;
			case ROTATION_90:  // 顺时针旋转90度: 新位置(i,j)的值来自原位置(3-j, i)
				src_i = 3 - j;
				src_j = i;
				break;
			case ROTATION_180:  // 旋转180度: 新位置(i,j)的值来自原位置(3-i, 3-j)
				src_i = 3 - i;
				src_j = 3 - j;
				break;
			case ROTATION_270:  // 顺时针旋转270度（逆时针90度）: 新位置(i,j)的值来自原位置(j, 3-i)
				src_i = j;
				src_j = 3 - i;
				break;
			default:  // 默认不旋转
				src_i = i;
				src_j = j;
				break;
			}
			temp_map[i][j] = default_key_map[src_i][src_j];
		}
	}

	// 再应用镜像（如果需要）
	if (mirror)
	{
		// 水平镜像：左右翻转 (i,j) -> (i, 3-j)
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 4; j++)
			{
				uint8_t dst_j = 3 - j;
				key_map[i][j] = temp_map[i][dst_j];
			}
		}
	}
	else
	{
		// 不镜像，直接复制
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 4; j++)
			{
				key_map[i][j] = temp_map[i][j];
			}
		}
	}
}

/**
 * @brief 初始化4x4矩阵键盘
 * @param rotation 顺时针旋转角度，0-360
 * @param mirror 是否水平镜像
 * @note 先旋转后镜像
 */
void Button4_4_Init(rotation_t rotation, bool mirror)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	unsigned char i;

	// 根据旋转和镜像参数生成key_map
	GenerateKeyMap(rotation, mirror);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);      // 重映射需要先使能AFIO时钟
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  // 只关闭JTAG而保留SWD

	RCC_APB2PeriphClockCmd(BUTTON_GPIO_CLK, ENABLE);

	for (i = 0; i < 4; i++)
	{
		GPIO_InitStructure.GPIO_Pin = KEY_OUT[i].GPIO_pin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

		GPIO_Init(KEY_OUT[i].GPIO_x, &GPIO_InitStructure);
	}

	for (i = 0; i < 4; i++)
	{
		GPIO_InitStructure.GPIO_Pin = KEY_IN[i].GPIO_pin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

		GPIO_Init(KEY_IN[i].GPIO_x, &GPIO_InitStructure);
	}

	for (i = 0; i < 4; i++)
	{
		GPIO_SetBits(KEY_OUT[i].GPIO_x, KEY_OUT[i].GPIO_pin);
	}
}

uint8_t Button4_4_Scan(void)
{

	for (uint8_t i = 0; i < 4; i++)
	{
		// 设置当前行为低电平
		GPIO_ResetBits(KEY_OUT[i].GPIO_x, KEY_OUT[i].GPIO_pin);

		// 短暂延时确保电平稳定
		delay_ms(5);

		// 扫描当前行的所有列
		for (uint8_t j = 0; j < 4; j++)
		{
			if (GPIO_ReadInputDataBit(KEY_IN[j].GPIO_x, KEY_IN[j].GPIO_pin) == 0)
			{
				// 检测到按键按下
				GPIO_SetBits(KEY_OUT[i].GPIO_x, KEY_OUT[i].GPIO_pin);
				return key_map[i][j];
			}
		}

		// 恢复当前行为高电平
		GPIO_SetBits(KEY_OUT[i].GPIO_x, KEY_OUT[i].GPIO_pin);
	}

	return 0xFF;  // 没有按键按下
}

// int Button4_4_Scan(void)
// {
//         unsigned char i, j;
//         for(i = 0; i < 4; i++)
//         {
//           delay_ms(5);
// 					GPIO_ResetBits(KEY_OUT[i].GPIO_x, KEY_OUT[i].GPIO_pin);
//           for(j = 0; j < 4; j++)
//           {
//                   delay_ms(5);
// 						      if(GPIO_ReadInputDataBit(KEY_IN[j].GPIO_x, KEY_IN[j].GPIO_pin) == 0)
//                    {
//                                 key[i][j] = 1;
//                    }else{
//                                 key[i][j] = 0;
//                    }
//           }
//           GPIO_SetBits(KEY_OUT[i].GPIO_x, KEY_OUT[i].GPIO_pin);
//         }
//         //键盘布局如下：
//         // 1 2 3 12
//         // 4 5 6 13
//         // 7 8 9 14
//         // 10 0 11 15
//              if(key[3][0]==1)return 1;
// 				else if(key[2][0]==1)return 2;
//         else if(key[1][0]==1)return 3;
//         else if(key[0][0]==1)return 12;
//         else if(key[3][1]==1)return 4;
// 				else if(key[2][1]==1)return 5;
//         else if(key[1][1]==1)return 6;
//         else if(key[0][1]==1)return 13;
//         else if(key[3][2]==1)return 7;
// 				else if(key[2][2]==1)return 8;
//         else if(key[1][2]==1)return 9;
//         else if(key[0][2]==1)return 14;
//         else if(key[3][3]==1)return 10;
// 				else if(key[2][3]==1)return 0;
//         else if(key[1][3]==1)return 11;
//         else if(key[0][3]==1)return 15;
// 				// if(key[0][0]==1)return 10;
//         // else if(key[0][1]==1)return 0;  //
//         // else if(key[0][2]==1)return 11;  //
//         // else if(key[1][0]==1)return 7;////
//         // else if(key[1][1]==1)return 8; //
//         // else if(key[1][2]==1)return 9;//
//         // else if(key[2][0]==1)return 4;   ///
//         // else if(key[2][1]==1)return 5;  //
//         // else if(key[2][2]==1)return 6;  ///
//         // else if(key[3][0]==1)return 1;   //
//         // else if(key[3][1]==1)return 2;  ///
//         // else if(key[3][2]==1)return 3;  //
//          return NULL;  // 没有按键按下时返回0
// //        if(key[0][0]==1)return 13;
// //        else if(key[0][1]==1)return 7;  //
// //        else if(key[0][2]==1)return 8;  //
// //        else if(key[0][3]==1)return 9;  //
// //        else if(key[1][1]==1)return 4;////
// //        else if(key[1][2]==1)return 5; //
// //        else if(key[1][3]==1)return 6;///
// //        else if(key[2][1]==1)return 1;   ///
// //        else if(key[2][2]==1)return 2;  //
// //        else if(key[2][3]==1)return 3;  ///
// //        else if(key[3][1]==1)return 10;   // **
// //        else if(key[3][2]==1)return 0;  ///
// //        else if(key[3][3]==1)return 11;  //##
// //
// //				else return 0;

// }