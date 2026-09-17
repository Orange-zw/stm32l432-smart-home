#ifndef __BL0942_H__
#define __BL0942_H__

#include <stdint.h>
#include "usart_factory.h"

#define MAX_C (10) // 电流最大值模块类型，你购买的是10A还是20A模块

typedef struct
{
    double V1;    // 有效电压
    double C1;    // 有效电流
    double P1;    // 有功功率
    double S1;    // 视在功率
    double PF1;   // 功率因数
    double E_con; // 已用电量
} BL0942_data_t;

void BL0942_Init(USART_InstanceType instance, uint32_t baud_rate);
void BL0942_Read_Data(BL0942_data_t *data);

#endif // __BL0942_H__