#ifndef __HC_SR505_H
#define __HC_SR505_H
#include "stm32f10x.h"
#include "delay.h"
#include "stdbool.h"

#define Have_body true // 有人
#define No_body false  // 无人

/***************根据自己需求更改引脚****************/
#define HC_SR505_GPIO_CLK RCC_APB2Periph_GPIOB
#define HC_SR505_GPIO_PORT GPIOB
#define HC_SR505_GPIO_PIN GPIO_Pin_13

void HC_SR505_Init(void);
bool Get_HC_SR505_Value(void);

#endif
