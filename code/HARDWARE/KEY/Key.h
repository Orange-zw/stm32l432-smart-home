#ifndef __KEY_H
#define __KEY_H
#include "sys.h"
void Key_Init(void);

u8 KEY_Scan(u8);  // 按键扫描函数

#define KEY1 PAin(12)  // 读取按键1
#define KEY2 PAin(15)  // 读取按键2
#define KEY3 PBin(3)   // 读取按键3
#define KEY4 PBin(4)   // 读取按键4
// #define KEY1 PBin(7) // 读取按键1
// #define KEY2 PBin(6) // 读取按键2
// #define KEY3 PBin(5) // 读取按键3
// #define KEY4 PBin(4) // 读取按键4

#define KEY1_PRES 1  // KEY1按下
#define KEY2_PRES 2  // KEY2按下
#define KEY3_PRES 3  // KEY3按下
#define KEY4_PRES 4  // KEY4按下

#endif
