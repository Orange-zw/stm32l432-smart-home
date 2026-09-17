#include "HW.h"

/* ============================================================
 * 模块     : 光电红外传感器
 * 功能     : 光电红外传感器c文件
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

void HW_Init(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;
		
		RCC_APB2PeriphClockCmd (HW_GPIO_CLK, ENABLE );	// 打开连接 传感器DO 的单片机引脚端口时钟
		GPIO_InitStructure.GPIO_Pin = HW_GPIO_PIN;			// 配置连接 传感器DO 的单片机引脚模式
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;			// 设置为上拉输入
		
		GPIO_Init(HW_GPIO_PORT, &GPIO_InitStructure);				// 初始化 
	
}

uint16_t HW_GetData(void)
{
	uint16_t tempData;
	tempData = !GPIO_ReadInputDataBit(HW_GPIO_PORT, HW_GPIO_PIN);
	return tempData;
}



