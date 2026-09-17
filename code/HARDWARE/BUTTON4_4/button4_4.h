#ifndef __BUTTON4_4_H
#define __BUTTON4_4_H
#include <stdbool.h>
#include <stdint.h>
#include "delay.h"
#include "math.h"
#include "stm32f10x.h"
extern u8 keynum;  // 按键值
/* ============================================================
 * 模块     : 4×4矩阵键盘
 * 功能     : 4×4矩阵键盘h文件
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

/***************根据自己需求更改****************/
// 4×4矩阵键盘 GPIO宏定义

#define BUTTON_GPIO_CLK RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA
#define BUTTON_ROW1_GPIO_PORT GPIOA
#define BUTTON_ROW1_GPIO_PIN GPIO_Pin_11

#define BUTTON_ROW2_GPIO_PORT GPIOA
#define BUTTON_ROW2_GPIO_PIN GPIO_Pin_12

#define BUTTON_ROW3_GPIO_PORT GPIOA
#define BUTTON_ROW3_GPIO_PIN GPIO_Pin_15

#define BUTTON_ROW4_GPIO_PORT GPIOB
#define BUTTON_ROW4_GPIO_PIN GPIO_Pin_3

#define BUTTON_COL1_GPIO_PORT GPIOB
#define BUTTON_COL1_GPIO_PIN GPIO_Pin_4

#define BUTTON_COL2_GPIO_PORT GPIOB
#define BUTTON_COL2_GPIO_PIN GPIO_Pin_5

#define BUTTON_COL3_GPIO_PORT GPIOB
#define BUTTON_COL3_GPIO_PIN GPIO_Pin_6

#define BUTTON_COL4_GPIO_PORT GPIOB
#define BUTTON_COL4_GPIO_PIN GPIO_Pin_7

/*********************END**********************/

typedef enum
{
	ROTATION_0 = 0,
	ROTATION_90,
	ROTATION_180,
	ROTATION_270,
	ROTATION_MAX,
} rotation_t;

void Button4_4_Init(rotation_t rotation, bool mirror);
uint8_t Button4_4_Scan(void);

#endif
