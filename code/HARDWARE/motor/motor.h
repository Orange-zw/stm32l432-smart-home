#ifndef __MOTOR_H
#define __MOTOR_H
#include "sys.h"

void TIM3_PWM_Init(uint16_t prescaler, uint16_t period);

void Set_TIM3_PWM_Duty(uint8_t channel, uint16_t duty);

void chuanfen(void);

void Servo_turn(u8 angle);

u8 Servo_getAngle(void);
#endif
