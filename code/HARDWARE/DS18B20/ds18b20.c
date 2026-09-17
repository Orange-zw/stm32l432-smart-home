#include "ds18b20.h"
#include "delay.h"

// 复位DS18B20
void DS18B20_Rst(void)
{
    DS18B20_IO_OUT();   // SET PG11 OUTPUT
    DS18B20_DQ_OUT = 0; // 拉低DQ
    delay_us(750);      // 拉低750us
    DS18B20_DQ_OUT = 1; // DQ=1
    delay_us(15);       // 15US
}
// 等待DS18B20的回应
// 返回1:未检测到DS18B20的存在
// 返回0:存在
u8 DS18B20_Check(void)
{
    u8 retry = 0;
    DS18B20_IO_IN(); // SET PG11 INPUT
    while (DS18B20_DQ_IN && retry < 200)
    {
        retry++;
        delay_us(1);
    };
    if (retry >= 200)
        return 1;
    else
        retry = 0;
    while (!DS18B20_DQ_IN && retry < 240)
    {
        retry++;
        delay_us(1);
    };
    if (retry >= 240)
        return 1;
    return 0;
}
// 从DS18B20读取一个位
// 返回值：1/0
u8 DS18B20_Read_Bit(void)
{
    u8 data;
    DS18B20_IO_OUT(); // SET PG11 OUTPUT
    DS18B20_DQ_OUT = 0;
    delay_us(2);
    DS18B20_DQ_OUT = 1;
    DS18B20_IO_IN(); // SET PG11 INPUT
    delay_us(12);
    if (DS18B20_DQ_IN)
        data = 1;
    else
        data = 0;
    delay_us(50);
    return data;
}
// 从DS18B20读取一个字节
// 返回值：读到的数据
u8 DS18B20_Read_Byte(void)
{
    u8 i, j, dat;
    dat = 0;
    for (i = 1; i <= 8; i++)
    {
        j = DS18B20_Read_Bit();
        dat = (j << 7) | (dat >> 1);
    }
    return dat;
}
// 写一个字节到DS18B20
// dat：要写入的字节
void DS18B20_Write_Byte(u8 dat)
{
    u8 j;
    u8 testb;
    DS18B20_IO_OUT(); // SET PG11 OUTPUT;
    for (j = 1; j <= 8; j++)
    {
        testb = dat & 0x01;
        dat = dat >> 1;
        if (testb)
        {
            DS18B20_DQ_OUT = 0; // Write 1
            delay_us(2);
            DS18B20_DQ_OUT = 1;
            delay_us(60);
        }
        else
        {
            DS18B20_DQ_OUT = 0; // Write 0
            delay_us(60);
            DS18B20_DQ_OUT = 1;
            delay_us(2);
        }
    }
}
// 开始温度转换
void DS18B20_Start(void)
{
    DS18B20_Rst();
    DS18B20_Check();
    DS18B20_Write_Byte(0xcc); // skip rom
    DS18B20_Write_Byte(0x44); // convert
}

// 初始化DS18B20的IO口 DQ 同时检测DS的存在
// 返回1:不存在
// 返回0:存在
u8 DS18B20_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    short temp;
    u8 i;

    RCC_APB2PeriphClockCmd(GPIO_CLOCK, ENABLE); // 使能PORTG口时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_PIN; // PORTG.11 推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_WHAT, &GPIO_InitStructure);

    GPIO_SetBits(GPIO_WHAT, GPIO_PIN); // 输出1

    DS18B20_Rst();

    // 检测传感器是否存在
    if (DS18B20_Check())
        return 1; // 传感器不存在

    // 初始化时进行几次预热读取，丢弃不稳定的数值
    // 启动一次温度转换
    DS18B20_Start();
    delay_ms(750); // 等待转换完成，DS18B20最长需要750ms完成转换

    // 进行3次读取以丢弃初始的不稳定值（如85℃）
    for (i = 0; i < 3; i++)
    {
        temp = DS18B20_Get_Temp();
        delay_ms(100);
    }

    return 0;
}

// 获取稳定的温度值，避免读取到初始的85℃
// 连续读取几次，返回最后一次读取的值
short DS18B20_Get_Stable_Temp(void)
{
    short temp;
    u8 i;

    // 进行多次读取以确保值稳定
    for (i = 0; i < 2; i++)
    {
        temp = DS18B20_Get_Temp();
        delay_ms(5);
    }

    return temp;
}

// 从ds18b20得到温度值
// 精度：0.1C
// 返回值：温度值 （-550~1250）
short DS18B20_Get_Temp(void)
{
    u8 temp;
    u8 TL, TH;
    short tem;
    DS18B20_Start(); // ds1820 start convert
    DS18B20_Rst();
    DS18B20_Check();
    DS18B20_Write_Byte(0xcc); // skip rom
    DS18B20_Write_Byte(0xbe); // convert
    TL = DS18B20_Read_Byte(); // LSB
    TH = DS18B20_Read_Byte(); // MSB

    if (TH > 7)
    {
        TH = ~TH;
        TL = ~TL;
        temp = 0; // 温度为负
    }
    else
        temp = 1; // 温度为正
    tem = TH;     // 获得高八位
    tem <<= 8;
    tem += TL;                // 获得底八位
    tem = (float)tem * 0.625; // 转换
    if (temp)
        return tem; // 返回温度值
    else
        return -tem;
}
