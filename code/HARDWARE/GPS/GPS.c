#include "sys.h"
#include "GPS.h"
#include "OLED.h"
// C库
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#define false 0
#define true 1

/********GPS变量************/
char rxdatabufer;
u16 point1 = 0;
struct SaveData Save_Data;
char dingwei_flag;
char Bluetooth_flag = 1;
extern char En_Message[100];
/********GPS变量************/

// 串口1中断服务程序
// 注意,读取USARTx->SR能避免莫名其妙的错误
u8 USART_RX_BUF[USART_REC_LEN]; // 接收缓冲,最大USART_REC_LEN个字节.

void GPS_Init(u32 bound)
{
#if GPS_USART == GPS_USART1
    // GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE); // 使能USART1，GPIOA时钟

    // USART1_TX   GPIOA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);          // 初始化GPIOA.9

    // USART1_RX	  GPIOA.10初始化
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;            // PA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);                // 初始化GPIOA.10

    // Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        // 子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);                           // 根据指定的参数初始化VIC寄存器

    // USART 初始化设置

    USART_InitStructure.USART_BaudRate = bound;                                     // 串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 // 收发模式

    USART_Init(USART1, &USART_InitStructure);      // 初始化串口1
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 开启串口接受中断
    USART_Cmd(USART1, ENABLE);                     // 使能串口1

#elif GPS_USART == GPS_USART2
    GPIO_InitTypeDef gpio_initstruct;
    USART_InitTypeDef usart_initstruct;
    NVIC_InitTypeDef nvic_initstruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // PA2	TXD
    gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_initstruct.GPIO_Pin = GPIO_Pin_2;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_initstruct);

    // PA3	RXD
    gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_initstruct.GPIO_Pin = GPIO_Pin_3;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_initstruct);

    usart_initstruct.USART_BaudRate = bound;
    usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
    usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 // 接收和发送
    usart_initstruct.USART_Parity = USART_Parity_No;                             // 无校验
    usart_initstruct.USART_StopBits = USART_StopBits_1;                          // 1位停止位
    usart_initstruct.USART_WordLength = USART_WordLength_8b;                     // 8位数据位
    USART_Init(USART2, &usart_initstruct);

    USART_Cmd(USART2, ENABLE); // 使能串口

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // 使能接收中断

    nvic_initstruct.NVIC_IRQChannel = USART2_IRQn;
    nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
    nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_initstruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&nvic_initstruct);

#elif GPS_USART == GPS_USART3
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // GPIOB时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    USART_DeInit(USART3); // 复位串?
    // USART2_TX   PB10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // PB10
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_Init(GPIOB, &GPIO_InitStructure);          // 初始化PB10

    // USART3_RX	  PB11
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
    GPIO_Init(GPIOB, &GPIO_InitStructure);                // 初始化PB11

    USART_InitStructure.USART_BaudRate = bound;                                     // 一般设置为9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 // 收发模式

    USART_Init(USART3, &USART_InitStructure); // 初始化串口	3

    USART_Cmd(USART3, ENABLE); // 使能串口
    // 使能接收中断
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); // 开启中断

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        // 子优先级0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);                           // 根据指定的参数初始化VIC寄存器
#endif
}

#if GPS_USART == GPS_USART1
/*↓↓↓↓↓↓↓↓↓↓↓↓GPS数据解析函数↓↓↓↓↓↓↓↓↓↓↓↓*/
void USART1_IRQHandler(void) // 串口1中断服务程序
{
    u8 Res;
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        Res = USART_ReceiveData(USART1); //(USART1->DR);	//读取接收到的数据

        if (Res == '$')
        {
            point1 = 0;
        }
        USART_RX_BUF[point1++] = Res;

        if (USART_RX_BUF[0] == '$' && USART_RX_BUF[4] == 'M' && USART_RX_BUF[5] == 'C') // 确定是否收到"GPRMC/GNRMC"这一帧数据
        {
            if (Res == '\n')
            {
                memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length); // 清空
                memcpy(Save_Data.GPS_Buffer, USART_RX_BUF, point1); // 保存数据
                Save_Data.isGetData = true;
                point1 = 0;
                memset(USART_RX_BUF, 0, USART_REC_LEN); // 清空
            }
        }
        if (point1 >= USART_REC_LEN)
        {
            point1 = USART_REC_LEN;
        }
    }
}
#elif GPS_USART == GPS_USART2
void USART2_IRQHandler(void) // 串口2中断服务程序
{
    u8 Res;
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        Res = USART_ReceiveData(USART2); //(USART2->DR);	//读取接收到的数据

        if (Res == '$')
        {
            point1 = 0;
        }
        USART_RX_BUF[point1++] = Res;

        if (USART_RX_BUF[0] == '$' && USART_RX_BUF[4] == 'M' && USART_RX_BUF[5] == 'C') // 确定是否收到"GPRMC/GNRMC"这一帧数据
        {
            if (Res == '\n')
            {
                memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length); // 清空
                memcpy(Save_Data.GPS_Buffer, USART_RX_BUF, point1); // 保存数据
                Save_Data.isGetData = true;
                point1 = 0;
                memset(USART_RX_BUF, 0, USART_REC_LEN); // 清空
            }
        }
        if (point1 >= USART_REC_LEN)
        {
            point1 = USART_REC_LEN;
        }
    }
}
#elif GPS_USART == GPS_USART3
void USART3_IRQHandler(void) // 串口3中断服务程序
{
    u8 Res;
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        Res = USART_ReceiveData(USART3); //(USART3->DR);	//读取接收到的数据

        if (Res == '$')
        {
            point1 = 0;
        }
        USART_RX_BUF[point1++] = Res;

        if (USART_RX_BUF[0] == '$' && USART_RX_BUF[4] == 'M' && USART_RX_BUF[5] == 'C') // 确定是否收到"GPRMC/GNRMC"这一帧数据
        {
            if (Res == '\n')
            {
                memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length); // 清空
                memcpy(Save_Data.GPS_Buffer, USART_RX_BUF, point1); // 保存数据
                Save_Data.isGetData = true;
                point1 = 0;
                memset(USART_RX_BUF, 0, USART_REC_LEN); // 清空
            }
        }
        if (point1 >= USART_REC_LEN)
        {
            point1 = USART_REC_LEN;
        }
    }
}
#endif

// 串口命令识别函数
u8 Hand(char *a)
{
    if (strstr((const char *)USART_RX_BUF, a) != NULL)
        return 1;
    else
        return 0;
}

// 串口缓存清理
void CLR_Buf(void)
{
    memset(USART_RX_BUF, 0, USART_REC_LEN); // 清空
    point1 = 0;
}

void clrStruct()
{
    Save_Data.isGetData = false;
    Save_Data.isParseData = false;
    Save_Data.isUsefull = false;
    memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length); // 清空
    memset(Save_Data.UTCTime, 0, UTCTime_Length);
    memset(Save_Data.latitude, 0, latitude_Length);
    memset(Save_Data.N_S, 0, N_S_Length);
    memset(Save_Data.longitude, 0, longitude_Length);
    memset(Save_Data.E_W, 0, E_W_Length);
}
void errorLog(int num)
{

    while (1)
    {
        //	  LCD_ShowChinese(48,  112,"解析错误",RED,WHITE,16,0);
    }
}

void parseGpsBuffer()
{
    char *subString;
    char *subStringNext;
    char i = 0;
    if (Save_Data.isGetData)
    {
        Save_Data.isGetData = false;

        for (i = 0; i <= 6; i++)
        {
            if (i == 0)
            {
                if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)
                    errorLog(1); // 解析错误
            }
            else
            {
                subString++;
                if ((subStringNext = strstr(subString, ",")) != NULL)
                {
                    char usefullBuffer[2];
                    switch (i)
                    {
                    case 1:
                        memcpy(Save_Data.UTCTime, subString, subStringNext - subString);
                        break; // 获取UTC时间
                    case 2:
                        memcpy(usefullBuffer, subString, subStringNext - subString);
                        break; // 获取UTC时间
                    case 3:
                        memcpy(Save_Data.latitude, subString, subStringNext - subString);
                        break; // 获取纬度信息
                    case 4:
                        memcpy(Save_Data.N_S, subString, subStringNext - subString);
                        break; // 获取N/S
                    case 5:
                        memcpy(Save_Data.longitude, subString, subStringNext - subString);
                        break; // 获取经度信息
                    case 6:
                        memcpy(Save_Data.E_W, subString, subStringNext - subString);
                        break; // 获取E/W

                    default:
                        break;
                    }

                    subString = subStringNext;
                    Save_Data.isParseData = true;
                    if (usefullBuffer[0] == 'A')
                    {
                        Save_Data.isUsefull = true;
                    }
                    else if (usefullBuffer[0] == 'V')
                    {
                        Save_Data.isUsefull = false;
                    }
                }
                else
                {
                    errorLog(2); // 解析错误
                }
            }
        }
    }
}

/*
 *==============================================================================
 *函数名称：Data_Transfor
 *函数功能：数据转换
 *输入参数：无
 *返回值：无
 *备  注：无
 *==============================================================================
 */
void GPS_Data_Transfor(float *latitude, float *longitude)
{
    u16 temp1 = 0; // 临时变量1，存储整数
    u16 temp2 = 0; // 临时变量2，存储整数

    *latitude = strtod(Save_Data.latitude, NULL);   // 字符串转换成浮点数
    *longitude = strtod(Save_Data.longitude, NULL); // 字符串转换成浮点数

    // 纬度信息处理
    // 五位纬度信息
    if ((*latitude - 10000.0) >= 0)
    {
        // 前三位需要单独拿出来组成一个数
        temp1 = (((u16)*latitude / 10000) % 10) * 100 + (((u16)*latitude / 1000) % 10) * 10 + ((u16)*latitude / 100) % 10;
        *latitude = *latitude - (float)temp1 * 100;
        *latitude = (float)temp1 + *latitude / 60;
    }
    else // 四位纬度信息
    {
        // 前两位需要单独拿出来组成一个数
        temp1 = (((u16)*latitude / 1000) % 10) * 10 + ((u16)*latitude / 100) % 10;
        *latitude = *latitude - (float)temp1 * 100;
        *latitude = (float)temp1 + *latitude / 60;
    }

    // 经度信息处理
    // 五位经度信息
    if ((*longitude - 10000.0) >= 0)
    {
        // 前三位需要单独拿出来组成一个数
        temp2 = (((u16)*longitude / 10000) % 10) * 100 + (((u16)*longitude / 1000) % 10) * 10 + ((u16)*longitude / 100) % 10;
        *longitude = *longitude - (float)temp2 * 100;
        *longitude = (float)temp2 + *longitude / 60;
    }
    else // 四位经度信息
    {
        // 前两位需要单独拿出来组成一个数
        temp2 = (((u16)*longitude / 1000) % 10) * 10 + ((u16)*longitude / 100) % 10;
        *longitude = *longitude - (float)temp2 * 100;
        *longitude = (float)temp2 + *longitude / 60;
    }
}

#define EARTH_RADIUS 6371.0 // 地球半径(km)
#define PI 3.14159265358979323846

/**
 * @brief  计算两个GPS坐标点之间的距离(单位:km)
 * @param  lon1: 第一点经度(度)
 * @param  lat1: 第一点纬度(度)
 * @param  lon2: 第二点经度(度)
 * @param  lat2: 第二点纬度(度)
 * @retval 距离(km)
 */
float GPS_Calculate_Distance(float lon1, float lat1, float lon2, float lat2)
{
    float dLat, dLon, a, c, distance;

    // 将角度转换为弧度
    lat1 = lat1 * PI / 180.0;
    lon1 = lon1 * PI / 180.0;
    lat2 = lat2 * PI / 180.0;
    lon2 = lon2 * PI / 180.0;

    // 计算纬度和经度的差值
    dLat = lat2 - lat1;
    dLon = lon2 - lon1;

    // Haversine公式
    a = sin(dLat / 2) * sin(dLat / 2) +
        cos(lat1) * cos(lat2) * sin(dLon / 2) * sin(dLon / 2);
    c = 2 * atan2(sqrt(a), sqrt(1 - a));
    distance = EARTH_RADIUS * c;

    return distance;
}

/*↑↑↑↑↑↑↑↑GPS相关函数↑↑↑↑↑↑↑↑↑↑↑↑*/
