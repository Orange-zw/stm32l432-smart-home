#include "mpudata.h"

#include "oled.h"
#include "stdio.h" //标准输入输出库

char tmp_buf[33];			//字符串数组
extern float pitch,roll,yaw; 		//欧拉角:俯仰角，偏航角，滚转角
extern short aacx,aacy,aacz;		//加速度传感器原始数据  angular acceleration
extern short gyrox,gyroy,gyroz;	//陀螺仪原始数据  gyroscope
int mpu_x_axis = 0, mpu_y_axis = 0;

unsigned char mpu_data[12] = {0};  //右边摇杆字符坐标

void MPUInit(void)
{
	MPU_Init();	              //初始化MPU6050
	while(mpu_dmp_init());    //初始化mpu_dmp库
}

void MPU_Read(void)
{	
	int temp = 0;
	if(mpu_dmp_get_data(&pitch,&roll,&yaw) == 0)//dmp处理得到数据，对返回值进行判断
	{ 	
		mpu_y_axis =pitch*10;
		if(mpu_y_axis > 400)
			mpu_y_axis = 400;

		if(mpu_y_axis < -400)
			mpu_y_axis = -400;

		
		//my_printf2("X:%d  ", mpu_y_axis);
		
		mpu_x_axis=-(roll*10); 
		if(mpu_x_axis > 400)
			mpu_x_axis = 400;

		if(mpu_x_axis < -400)
			mpu_x_axis = -400;
		
		//my_printf2("Y:%d\n", mpu_x_axis);
		
		temp=pitch*10;							 //赋temp为pitch
		if(temp<0)								//对数据正负判断，判断为负时
		{
			temp=-temp;	 
		  sprintf((char *)tmp_buf," -%d.%d",temp/10,temp%10);
		  OLED_ShowString(0,7,(u8 *)tmp_buf,8);			//对负数据取反		
		}
		else                                    //判断为正时 
		{
			sprintf((char *)tmp_buf,"  %d.%d",temp/10,temp%10);
		   OLED_ShowString(0,7,(u8 *)tmp_buf,8);
		}	
		
		temp=roll*10;                            //赋temp为roll
		if(temp<0)								//对数据正负判断，判断为负时
		{
			temp=-temp;
			sprintf((char *)tmp_buf," -%d.%d",temp/10,temp%10);
		  OLED_ShowString(90,7,(u8 *)tmp_buf,8);						    //对负数据取反	
		}
		else                                    //判断为正时
		{
			sprintf((char *)tmp_buf,"  %d.%d",temp/10,temp%10);
		  OLED_ShowString(90,7,(u8 *)tmp_buf,8);
		}
	}
}



