#include "HC_SR505.h"

void HC_SR505_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(HC_SR505_GPIO_CLK, ENABLE); // 打开连接 传感器DO 的单片机引脚端口时钟
    GPIO_InitStructure.GPIO_Pin = HC_SR505_GPIO_PIN;   // 配置连接 传感器DO 的单片机引脚模式
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;      // 设置为下拉输入

    GPIO_Init(HC_SR505_GPIO_PORT, &GPIO_InitStructure); // 初始化
}

bool Get_HC_SR505_Value(void)
{
    uint16_t tempData;
    tempData = GPIO_ReadInputDataBit(HC_SR505_GPIO_PORT, HC_SR505_GPIO_PIN);
    if (tempData == 1)
    {
        return Have_body;
    }
    else
    {
        return No_body;
    }
}
