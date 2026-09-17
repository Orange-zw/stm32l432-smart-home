#ifndef __LED_H
#define __LED_H
#include "sys.h"

#define ON 1  // 开
#define OFF 0 // 关

#define BEEP PCout(13) // 蜂鸣器
#define BEEP_ON() (BEEP = OFF)
#define BEEP_OFF() (BEEP = ON)

#define Motor PBout(15)      // 指示灯

void LED_Init(void);

#endif
