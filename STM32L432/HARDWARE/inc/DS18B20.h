#ifndef __DS18B20_H
#define __DS18B20_H 

#include "sys.h"
#include "delay.h"	  
#include "usart.h"

#define DS18B20_DQ_OUT_SET  GPIOB->ODR|=(1<<5)
#define DS18B20_DQ_OUT_RESET  GPIOB->ODR&=~(1<<5)
#define DS18B20_DQ_IN  (GPIOB->IDR&=(1<<5))

#define DS18B20_Clock RCC_AHB1Periph_GPIOB
#define DS18B20_GPIO  GPIOB
#define DS18B20_Pin		GPIO_Pin_5


//IO方向设置
#define DS18B20_IO_IN()  {DS18B20_GPIO->MODER&=~(3<<(5*2));DS18B20_GPIO->MODER|=0<<5*2;}	//PB5输入模式
#define DS18B20_IO_OUT() {DS18B20_GPIO->MODER&=~(3<<(5*2));DS18B20_GPIO->MODER|=1<<5*2;} 	//PB5输出模式
 
extern float DS18B20_Temperature; 

u8 DS18B20_Init(void);			//初始化DS18B20
double DS18B20_Get_Temp(void);	//获取温度
void DS18B20_Start(void);		//开始温度转换
void DS18B20_Write_Byte(u8 dat);//写入一个字节
u8 DS18B20_Read_Byte(void);		//读出一个字节
u8 DS18B20_Read_Bit(void);		//读出一个位
u8 DS18B20_Check(void);			//检测是否存在DS18B20
void DS18B20_Rst(void);			//复位DS18B20    

#endif















