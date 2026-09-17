#include "stm32f10x.h"
#include "PM2_5.h"
#include "delay.h"

void PM25_Init(void)
{
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_APB2PeriphClockCmd(PM25_ADC_GPIO_RCC, ENABLE);
  RCC_APB2PeriphClockCmd(PM25_LED_GPIO_RCC, ENABLE);

  RCC_ADCCLKConfig(RCC_PCLK2_Div6);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Pin = PM25_ADC_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(PM25_ADC_GPIO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = PM25_LED_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 速度50MHz
  GPIO_Init(PM25_LED_GPIO_PORT, &GPIO_InitStructure);

  ADC_InitTypeDef ADC_InitStructure;
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  ADC_Cmd(ADC1, ENABLE);

  ADC_ResetCalibration(ADC1);
  while (ADC_GetResetCalibrationStatus(ADC1) == SET)
    ;
  ADC_StartCalibration(ADC1);
  while (ADC_GetCalibrationStatus(ADC1) == SET)
    ;
}

uint16_t PM25_GetValue(void)
{
  ADC_RegularChannelConfig(ADC1, PM25_ADC_CHANNEL, 1, ADC_SampleTime_55Cycles5);
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
  while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
    ;
  return ADC_GetConversionValue(ADC1);
}

unsigned int PM2_5_GetData(void) // 定义读取PM2.5的函数
{
  static u16 adc_value = 0;
  static u16 PM_count = 0;
  static unsigned int last_pm25 = 0;
  unsigned int PM25_value = last_pm25;
  PM25_LED = 0;
  delay_us(280);
  adc_value += PM25_GetValue();
  delay_us(40);
  PM25_LED = 1;
  if (++PM_count >= 3) // 获取5次adc值
  {
    PM_count = 0;              // 计数次数清零
    adc_value = adc_value / 5; // 取5次平均值
    {
      float v = (float)adc_value;
      float pm = ((v * 3.3f) / 4095.0f) * 1000.0f * 0.17f - 0.1f;
      if (pm < 0.0f)
        pm = 0.0f;
      PM25_value = (unsigned int)(pm + 0.5f);
      last_pm25 = PM25_value;
    }
    adc_value = 0; // adc值清零
  }
  return last_pm25;
}
