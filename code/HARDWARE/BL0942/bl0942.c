#include "bl0942.h"

static USART_handle_t bl0942_usart = NULL;
static USART_Operations *ops = NULL;
static uint8_t rx_buf[128];
static uint16_t rx_len = 0;
static uint8_t rx_flag = 0;

void Data_Process(unsigned char *data, BL0942_data_t *ret);
void Clear_buf();

void BL0942_rx_callback(uint8_t *data, uint16_t length)
{
    memcpy(rx_buf, data, length);
    rx_len = length;
}

void BL0942_Init(USART_InstanceType instance, unsigned int bound)
{
    bl0942_usart = USARTFactory_Create(instance, bound);
    ops = USART_GetOperations();
    if (bl0942_usart != NULL && ops != NULL)
    {
        ops->init(bl0942_usart);
    }
    ops->set_rx_idle_callback(bl0942_usart, BL0942_rx_callback);
}

void BL0942_Read_Data(BL0942_data_t *data)
{
    uint8_t cmd[2] = {0x58, 0xAA};
    ops->send(bl0942_usart, cmd, 2);
    Clear_buf();

    while (rx_len < 23) // 返回数据包长度为23byte
        ;

    Data_Process(rx_buf, data);
    return;
}

static void Clear_buf()
{
    memset(rx_buf, 0, sizeof rx_buf);
    rx_len = 0;
}

/*
数据解析函数
*/
static void Data_Process(unsigned char *data, BL0942_data_t *ret)
{
    u8 i = 0, check_num = 0;
    u32 count = 88;
    u32 V_REG = 0, P_REG = 0, PF_COUNT = 0;
    int32_t C_REG = 0;

    char oled_str[100]; // OLED显示缓存
    char date[200];     // esp8266发送缓存

    for (i = 0; i < 22; i++) // 求和，用来计算校验码
    {
        count = count + data[i];
    }

    check_num = ~(count & 0xFF); // 取最后一个字节，然后按位取反

    if (check_num == data[22]) // 校验数据是正确
    {
        C_REG = data[3] * 65536 + data[2] * 256 + data[1]; // 计算电流寄存器
        if (data[3] & 0x80)                                // 高字节的最高位如果为1，说明电流为负数
        {
            C_REG = -(16777216 - C_REG);
        }
        if (MAX_C == 10) // 电流最大值模块类型，你购买的是10A还是20A模块
        {
            ret->C1 = C_REG * 1.218 / (305978 * 3); // 计算有效电流
        }
        else
        {
            ret->C1 = C_REG * 1.218 / (305978); // 计算有效电流
        }

        V_REG = data[6] * 65536 + data[5] * 256 + data[4]; // 计算电压寄存器
        ret->V1 = V_REG * 1.218 * 1950.51 / 37734390;      // 计算有效电压

        ret->S1 = ret->V1 * ret->C1; // P2为视在功率，视在功率=有效电压*有效电流

        P_REG = data[12] * 65536 + data[11] * 256 + data[10]; // 计算有功功率寄存器
        if (data[12] & 0x80)                                  // 高字节的最高位如果为1，说明有功功率为负数
        {
            P_REG = -(16777216 - P_REG);
        }
        if (MAX_C == 10) // 电流最大值模块类型，你购买的是10A还是20A模块
        {
            double temp = P_REG * 1.218 * 1.218 * 1950.51 / 5411610; // 计算有功功率
            ret->P1 = temp;
        }
        else
        {
            double temp = P_REG * 1.218 * 1.218 * 1950.51 / 1803870; // 计算有功功率
            ret->P1 = temp;
        }
        if (ret->S1 != 0)
            ret->PF1 = ret->P1 / ret->S1; // 计算功率因数；功率因数=有功功率/视在功率

        PF_COUNT = data[15] * 65536 + data[14] * 256 + data[13]; // 计算已用电量脉冲数
        if (MAX_C == 10)                                         // 电流最大值模块类型，你购买的是10A还是20A模块
        {
            ret->E_con = PF_COUNT / 16051.896; // 计算已用电量，16051.896为固定值，和选购的量程有关
        }
        else
        {
            ret->E_con = PF_COUNT / 5350.632; // 计算已用电量，5350.632为固定值，和选购的量程有关
        }
    }
    else
    {
        //		printf("Check Error\r\n");//校验数据有误，校验数据出错
    }
}
