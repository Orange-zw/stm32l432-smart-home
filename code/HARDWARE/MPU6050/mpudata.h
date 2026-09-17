#ifndef __MPUDATA_H
#define __MPUDATA_H
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "mpu6050.h" //MPU6050������
#include "inv_mpu.h" //������������
#include "inv_mpu_dmp_motion_driver.h" //DMP��̬�����

void MPUInit(void);
void MPU_Read(void);
void SendMpuData(void);
void MpuDataProcessing(void)	;

#endif



