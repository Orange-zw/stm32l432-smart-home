/************************************************************************************

*************************************************************************************/
#include "HX711.h"
#include "delay.h"

u32 HX711_Buffer;
u32 Weight_Maopi;
s32 Weight_Shiwu;
u8 Flag_Error = 0;
// 校准方法：先把Get_Weight函数里的-21830删除，更改GapValue，进行校准后再减
// 校准参数
// 因为不同的传感器特性曲线不是很一致，因此，每一个传感器需要矫正这里这个参数才能使测量值很准确。
// 当发现测试出来的重量偏大时，增加该数值。
// 如果测试出来的重量偏小时，减小改数值。
// 该值可以为小数
#define GapValue 400

void Init_HX711pin(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // 使能PF端口时钟

	// HX711_SCK
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;		  // 端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // IO口速度为50MHz
	GPIO_Init(GPIOB, &GPIO_InitStructure);			  // 根据设定参数初始化GPIOB

	// HX711_DOUT
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;	  // 端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 输入上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_SetBits(GPIOB, GPIO_Pin_6); // 初始化设置为1
}

//****************************************************
// 读取HX711
//****************************************************
u32 HX711_Read(void) // 增益128
{
	unsigned long count;
	unsigned char i;
	HX711_DOUT = 1;
	delay_us(1);
	HX711_SCK = 0;
	count = 0;
	while (HX711_DOUT)
		;
	for (i = 0; i < 24; i++)
	{
		HX711_SCK = 1;
		count = count << 1;
		delay_us(1);
		HX711_SCK = 0;
		if (HX711_DOUT)
			count++;
		delay_us(1);
	}
	HX711_SCK = 1;
	count = count ^ 0x800000; // 第25个脉冲下降沿来时，转换数据
	delay_us(1);
	HX711_SCK = 0;
	return (count);
}

//****************************************************
// 获取毛皮重量
//****************************************************
void Get_Maopi(void)
{
	uint32_t sum = 0;
	for (uint8_t i = 0; i < 10; i++)
	{
		sum += HX711_Read();
		delay_ms(10);
	}

	Weight_Maopi = sum / 10;
}

//****************************************************
// 称重
//****************************************************
uint32_t Get_Weight(void)
{
	// HX711_Buffer = HX711_Read();
	// if (HX711_Buffer > Weight_Maopi)
	// {
	// 	Weight_Shiwu = HX711_Buffer;
	// 	Weight_Shiwu = Weight_Shiwu - Weight_Maopi; // 获取实物的AD采样数值。

	// 	Weight_Shiwu = (s32)((float)Weight_Shiwu / GapValue); // 计算实物的实际重量
	// 														  // 因为不同的传感器特性曲线不一样，因此，每一个传感器需要矫正这里的GapValue这个除数。
	// 														  // 当发现测试出来的重量偏大时，增加该数值。
	// 														  // 如果测试出来的重量偏小时，减小改数值。
	// 														  // Weight_Shiwu = Weight_Shiwu - 20748 - 755 - 5;
	// }
	// if ((u32)Weight_Shiwu > 20748)
	// {
	// 	Weight_Shiwu = 0;
	// }
	Weight_Shiwu = (HX711_Read() - Weight_Maopi) / GapValue - 90; // 校准值90
	if (Weight_Shiwu > 10 * 1000 || Weight_Shiwu < 0)			  // 超过10kg则认为是错误数据
	{
		Weight_Shiwu = 0;
	}
	return (u32)Weight_Shiwu;
}
