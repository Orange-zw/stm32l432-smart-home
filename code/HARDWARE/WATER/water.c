#include "water.h"


//void WATER_Init(void)
//{
//	#if MODE
//	{
//		GPIO_InitTypeDef GPIO_InitStructure;
//		
//		RCC_APB2PeriphClockCmd (WATER_AO_GPIO_CLK, ENABLE );	// 打开 ADC IO端口时钟
//		GPIO_InitStructure.GPIO_Pin = WATER_AO_GPIO_PIN;					// 配置 ADC IO 引脚模式
//		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		// 设置为模拟输入
//		
//		GPIO_Init(WATER_AO_GPIO_PORT, &GPIO_InitStructure);				// 初始化 ADC IO

//		ADCx_Init();
//	}
//	#else
//	{
//		GPIO_InitTypeDef GPIO_InitStructure;
//		
//		RCC_APB2PeriphClockCmd (WATER_DO_GPIO_CLK, ENABLE );	// 打开连接 传感器DO 的单片机引脚端口时钟
//		GPIO_InitStructure.GPIO_Pin = WATER_DO_GPIO_PIN;			// 配置连接 传感器DO 的单片机引脚模式
//		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;			// 设置为上拉输入
//		
//		GPIO_Init(WATER_DO_GPIO_PORT, &GPIO_InitStructure);				// 初始化 
//		
//	}
//	#endif
//	
//}

//#if MODE
//uint16_t WATER_ADC_Read(void)
//{
//	//设置指定ADC的规则组通道，采样时间
//	return ADC_GetValue(ADC_CHANNEL, ADC_SampleTime_55Cycles5);
//}
//#endif

//uint16_t WATER_GetData(void)
//{
//	
//	#if MODE
//	uint32_t  tempData = 0;
//	for (uint8_t i = 0; i < WATER_READ_TIMES; i++)
//	{
//		tempData += WATER_ADC_Read();
//		delay_ms(5);
//	}

//	tempData /= WATER_READ_TIMES;
//	return tempData;
//	
//	#else
//	uint16_t tempData;
//	tempData = GPIO_ReadInputDataBit(WATER_DO_GPIO_PORT, WATER_DO_GPIO_PIN);
//	return tempData;
//	#endif
//}


void AD_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//开启ADC1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
	
	/*设置ADC时钟*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						//选择时钟6分频，ADCCLK = 72MHz / 6 = 12MHz
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA0引脚初始化为模拟输入
	
	/*规则组通道配置*/
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);		//规则组序列1的位置，配置为通道0
	
	/*ADC初始化*/
	ADC_InitTypeDef ADC_InitStructure;						//定义结构体变量
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;		//模式，选择独立模式，即单独使用ADC1
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//数据对齐，选择右对齐
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//外部触发，使用软件触发，不需要外部触发
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;		//连续转换，失能，每转换一次规则组序列后停止
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;			//扫描模式，失能，只转换规则组的序列1这一个位置
	ADC_InitStructure.ADC_NbrOfChannel = 1;					//通道数，为1，仅在扫描模式下，才需要指定大于1的数，在非扫描模式下，只能是1
	ADC_Init(ADC1, &ADC_InitStructure);						//将结构体变量交给ADC_Init，配置ADC1
	
	/*ADC使能*/
	ADC_Cmd(ADC1, ENABLE);									//使能ADC1，ADC开始运行
	
	/*ADC校准*/
	ADC_ResetCalibration(ADC1);								//固定流程，内部有电路会自动执行校准
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
}

/**
  * 函    数：获取AD转换的值
  * 参    数：无
  * 返 回 值：AD转换的值，范围：0~4095
  */
uint16_t AD_GetValue(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);					//软件触发AD转换一次
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);	//等待EOC标志位，即等待AD转换结束
	return ADC_GetConversionValue(ADC1);					//读数据寄存器，得到AD转换的结果
}

