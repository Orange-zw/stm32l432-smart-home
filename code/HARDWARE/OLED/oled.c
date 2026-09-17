/* ============================================================
 * 模块     : OLED 显示驱动
 * 功能     : SSD1306 屏的显存管理与字符/图形绘制
 * 作者     : Orange-zw
 * MCU      : STM32F103C8T6
 * 说明     : 智能家居控制系统（离线语音 + 云端远程控制）
 * ============================================================ */

#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"
#include "delay.h"

// OLED的显存
// 存放格式如下.
//[0]0 1 2 3 ... 127
//[1]0 1 2 3 ... 127
//[2]0 1 2 3 ... 127
//[3]0 1 2 3 ... 127
//[4]0 1 2 3 ... 127
//[5]0 1 2 3 ... 127
//[6]0 1 2 3 ... 127
//[7]0 1 2 3 ... 127
#define NOP() delay_us(1)
/*短暂延时*/
void I2C_delay(void)
{
    /*　
        下面的时间是通过逻辑分析仪测试得到的。
        CPU主频72MHz时，在内部Flash运行, MDK工程不优化
        循环次数为10时，SCL频率 = 205KHz
        循环次数为7时，SCL频率 = 347KHz， SCL高电平时间1.5us，SCL低电平时间2.87us
        循环次数为5时，SCL频率 = 421KHz， SCL高电平时间1.25us，SCL低电平时间2.375us

    IAR工程编译效率高，不能设置为7
    */
              for (u8 i = 0; i < 10; i++)
        ;
}
#if (OLED_MODE == OLED_IIC)
/**
 * 函数功能: CPU发起I2C总线启动信号
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明：无
 */
void OLED_IIC_Start()
{
    OLED_SDA = 1;
    OLED_SCL = 1;
    I2C_delay();
    OLED_SDA = 0;
    I2C_delay();
    OLED_SCL = 0;
    I2C_delay();
}
/**
 * 函数功能: CPU发起I2C总线停止信号
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明：无
 */
void OLED_IIC_Stop()
{
    OLED_SDA = 0;
    OLED_SCL = 1;
    I2C_delay();
    OLED_SDA = 1;
}

/**
 * 函数功能: CPU产生一个时钟，并读取器件的ACK应答信号
 * 输入参数: 无
 * 返 回 值: 返回0表示正确应答，1表示无器件响应
 * 说    明：无
 */
u8 OLED_I2C_WaitAck(void)
{
    uint8_t re;

    OLED_SDA = 1; /* CPU释放SDA总线 */
    I2C_delay();
    OLED_SCL = 1; /* CPU驱动SCL = 1, 此时器件会返回ACK应答 */
    I2C_delay();
    if (I2C_SDA_READ()) /* CPU读取SDA口线状态 */
    {
        re = 1;
    }
    else
    {
        re = 0;
    }
    OLED_SCL = 0;
    I2C_delay();
    return re;
}

/**********************************************
// 通过I2C总线写一个字节
**********************************************/
void OLED_Write_IIC_Byte(unsigned char IIC_Byte)
{
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        if (IIC_Byte >> 7)
        {
            OLED_SDA = 1;
        }
        else
        {
            OLED_SDA = 0;
        }
        I2C_delay();
        IIC_Byte <<= 1;
        OLED_SCL = 1;
        I2C_delay();
        OLED_SCL = 0;
        if (i == 7)
        {
            OLED_SDA = 1; // 释放总线
        }
        I2C_delay();
    }
}
// 向SSD1106写入一个字节。
// dat:要写入的数据/命令
// cmd:数据/命令标志 0,表示命令;1,表示数据;
void OLED_WR_Byte(u8 dat, u8 cmd)
{
    OLED_IIC_Start();
    OLED_Write_IIC_Byte(0x78);
    OLED_I2C_WaitAck();
    if (cmd)
    {
        OLED_Write_IIC_Byte(0x40); // write data
    }
    else
    {
        OLED_Write_IIC_Byte(0x00); // write command
    }
    OLED_I2C_WaitAck();
    OLED_Write_IIC_Byte(dat);
    OLED_I2C_WaitAck();
    OLED_IIC_Stop();
}
#elif (OLED_MODE == OLED_SPI)
// 向SSD1106写入一个字节。
// dat:要写入的数据/命令
// cmd:数据/命令标志 0,表示命令;1,表示数据;
void OLED_WR_Byte(u8 dat, u8 cmd)
{
    u8 i;
    if (cmd)
    {
        OLED_DC_Set();
    }
    else
    {
        OLED_DC_Clr();
    }
    OLED_CS_Clr();
    for (i = 0; i < 8; i++)
    {
        OLED_SCLK_Clr();
        if (dat & 0x80)
        {
            OLED_SDIN_Set();
        }
        else
        {
            OLED_SDIN_Clr();
        }
        OLED_SCLK_Set();
        dat <<= 1;
    }
    OLED_CS_Set();
    OLED_DC_Set();
}
#endif
void OLED_Set_Pos(u8 x, u8 y)
{
    OLED_WR_Byte(0xb0 + y, OLED_CMD);
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
    OLED_WR_Byte((x & 0x0f) | 0x01, OLED_CMD);
}
// 开启OLED显示
void OLED_Display_On(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); // SET DCDC命令
    OLED_WR_Byte(0X14, OLED_CMD); // DCDC ON
    OLED_WR_Byte(0XAF, OLED_CMD); // DISPLAY ON
}
// 关闭OLED显示
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); // SET DCDC命令
    OLED_WR_Byte(0X10, OLED_CMD); // DCDC OFF
    OLED_WR_Byte(0XAE, OLED_CMD); // DISPLAY OFF
}
// 清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!
void OLED_Clear(u8 mode)
{
    u8 i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD); // 设置页地址（0~7）
        OLED_WR_Byte(0x00, OLED_CMD);     // 设置显示位置―列低地址
        OLED_WR_Byte(0x10, OLED_CMD);     // 设置显示位置―列高地址
        for (n = 0; n < 128; n++)
        {
            OLED_WR_Byte(0, OLED_DATA);
        }
    } // 更新显示
}
/********************************************************************************
* @函数名：         OLED_ShowChar
* @函数描述：       在指定位置显示占宽8*16的单个ASCII字符
* @输入参数：
                    参数名    参数类型  参数描述
                    @x：      u8         列坐标，0~127
                    @y：      u8         行坐标，0~7
                    @chr：    u8         要显示的ASCII字符
                    @flag：   u8         反白标志，非0时反白显示
* @返回值：         void
* @其他：
********************************************************************************/
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 flag)
{
    unsigned char c = 0, i = 0;
    c = chr - ' '; // 得到偏移后的值
    if (x > Max_Column - 1)
    {
        x = 0;
        y = y + 2;
    }
    OLED_Set_Pos(x, y);
    for (i = 0; i < 8; i++)
    {
        if (flag == 1)
        {
            OLED_WR_Byte(~F8X16[c * 16 + i], OLED_DATA);
        }
        else
        {
            OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
        }
    }
    OLED_Set_Pos(x, y + 1);
    for (i = 0; i < 8; i++)
    {
        if (flag == 1)
        {
            OLED_WR_Byte(~F8X16[c * 16 + i + 8], OLED_DATA);
        }
        else
        {
            OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
        }
    }
}
// m^n函数
u32 oled_pow(u8 m, u8 n)
{
    u32 result = 1;
    while (n--)
    {
        result *= m;
    }
    return result;
}

/********************************************************************************
* @函数名：         OLED_ShowCHineseWord
* @函数描述：       在指定位置显示占宽16*16的单个 汉字
* @输入参数：
                    参数名    参数类型  参数描述
                    @x：      u8         列坐标，0~127
                    @y：      u8         行坐标，0~7
                    @str：    u8*        要显示的汉字
                    @flag：   u8         反白标志，非0时反白显示
* @返回值：         void
* @其他：
********************************************************************************/
void OLED_ShowCHineseWord(u8 x, u8 y, u8 *str, u8 flag)
{
    u8 t = 0;
    u16 index;
    for (index = 0; index < sizeof(CHINESE_CODE) / 35; index++)
    {
        if (CHINESE_CODE[index].Text[0] == str[0] && CHINESE_CODE[index].Text[1] == str[1]) // 对比汉字区码位码
        {
            OLED_Set_Pos(x, y);      // 设置OLED光标位置
            for (t = 0; t < 16; t++) // 先写汉字上半部分数据
            {
                if (flag == 0)
                {
                    OLED_WR_Byte(CHINESE_CODE[index].Code[t], OLED_DATA);
                }
                else
                {
									OLED_WR_Byte(~CHINESE_CODE[index].Code[t], OLED_DATA);
                }
            }
            OLED_Set_Pos(x, y + 1);  // 设置OLED光标位置
            for (t = 0; t < 16; t++) // 再写汉字下半部分数据
            {
                if (flag == 0)
                {
                    OLED_WR_Byte(CHINESE_CODE[index].Code[t + 16], OLED_DATA);
                }
                else
                {
                    OLED_WR_Byte(~CHINESE_CODE[index].Code[t + 16], OLED_DATA);
                }
            }
            break;
        }
    }
}

/********************************************************************************
* @函数名：         OLED_ShowText
* @函数描述：       在指定位置显示中英文字符串
* @输入参数：
                    参数名    参数类型  参数描述
                    @x：      u8         列坐标，0~127
                    @y：      u8         行坐标，0~7
                    @str：    u8*        要显示的字符串
                    @flag：   u8         反白标志，非0时反白显示
* @返回值：         void
* @其他：
********************************************************************************/
void OLED_ShowText(u8 x, u8 y, u8 *str, u8 flag)
{
    u8 tempstr[3] = {'\0'};

    while ((*str) != '\0')
    {
        if ((*str) & 0x80) // 判断下一个字符是中文还是英文
        {
            tempstr[0] = *str++;
            tempstr[1] = *str++;
            OLED_ShowCHineseWord(x, y, tempstr, flag);
            x += 16;
            if (x > 128 - 16)
            {
                // y++; // 修改地址
                // y++;
                // x = 0;
                break; // 超出128宽度后退出
            }
        }
        else
        {
            OLED_ShowChar(x, y, *str++, flag);
            x += 8;
            if (x > 128 - 8)
            {
                // y++; // 修改地址
                // y++;
                // x = 0;
                break; // 超出128宽度后退出
            }
        }
    }
    //	HAL_TIM_Base_Start_IT(&htim1);
    //	HAL_TIM_Base_Start_IT(&htim2);
}

void OLED_ShowTextLine(u8 line, u8 x, u8 *str, u8 flag)
{
    if (line > 4)
        line = 4;
    uint8_t y = line * 2;
    OLED_ShowText(x, y, str, flag);
}

void OLED_ClearLine(u8 line, u8 x, u8 flag)
{
    uint8_t str[128] = "##########";
    uint8_t y = line * 2;
    OLED_ShowTextLine(line, x, str, flag);
}

/***********功能描述：显示显示BMP图片128×64起始点坐标(x,y),x的范围0～127，y为页的范围0～7*****************/
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, const unsigned char BMP[])
{
    unsigned int j = 0;
    unsigned char x, y;
    if (y1 % 8 == 0)
    {
        y = y1 / 8;
    }
    else
    {
        y = y1 / 8 + 1;
    }
    for (y = y0; y < y1; y++)
    {
        OLED_Set_Pos(x0, y);
        for (x = x0; x < x1; x++)
        {
            OLED_WR_Byte(BMP[j++], OLED_DATA);
        }
    }
}

/**
 * @brief 显示一个字符串
 * @param x,y : 起始点坐标(x:0~127, y:0~7)
 * @param *p : 字符串起始地址
 * @param size : 字符大小(字号16)
 * @return None
 */
void OLED_ShowString(u8 x, u8 y, u8 *p, u8 size)
{
    // 忽略size参数，本驱动仅支持16号字
    while (*p != '\0')
    {
        OLED_ShowChar(x, y, *p, 0);
        x += 8;
        p++;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
    }
}

/**
 * @brief 显示数字
 * @param x,y : 起始坐标
 * @param num : 要显示的数字
 * @param len : 数字的位数
 * @return None
 */
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len)
{
    u8 t, temp;
    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        OLED_ShowChar(x + 8 * t, y, temp + '0', 0);
    }
}

/**
 * @brief 显示汉字
 * @param x,y : 起始坐标
 * @param no : 汉字在oledfont.h中的索引
 * @return None
 */
void OLED_ShowChinese(u8 x, u8 y, u8 no)
{
    u8 str[3];

    // 根据索引从CHINESE_CODE中找到对应汉字
    str[0] = CHINESE_CODE[no].Text[0];
    str[1] = CHINESE_CODE[no].Text[1];
    str[2] = '\0';

    OLED_ShowCHineseWord(x, y, str, 0);
}

// 初始化SSD1306
void OLED_Init(void)
{
    const u8 OLED_ConfigCmd[] =
        {
            0xAE, // display off
            0x20, // Set Memory Addressing Mode
            0x10, // 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
            0xb0, // Set Page Start Address for Page Addressing Mode,0-7
            0xc8, // Set COM Output Scan Direction
            0x00, //---set low column address
            0x10, //---set high column address
            0x40, //--set start line address
            0x81, //--set contrast control register
            0xdf, // 亮度调节 0x00~0xff
            0xa1, //--set segment re-map 0 to 127
            0xa6, //--set normal display
            0xa8, //--set multiplex ratio(1 to 64)
            0x3f, //
            0xa4, // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
            0xd3, //-set display offset
            0x00, //-not offset
            0xd5, //--set display clock divide ratio/oscillator frequency
            0xf0, //--set divide ratio
            0xd9, //--set pre-charge period
            0x22, //
            0xda, //--set com pins hardware configuration
            0x12,
            0xdb, //--set vcomh
            0x20, // 0x20,0.77xVcc
            0x8d, //--set DC-DC enable
            0x14, //
            0xaf, //--turn on oled panel
        };
#if (OLED_MODE == OLED_IIC)

    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // 配置SCL引脚
    GPIO_InitStructure.GPIO_Pin = OLED_SCL_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SCL_GPIO_Port, &GPIO_InitStructure);

    // 配置SDA引脚
    GPIO_InitStructure.GPIO_Pin = OLED_SDA_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SDA_GPIO_Port, &GPIO_InitStructure);

    // 设置引脚初始电平
    GPIO_SetBits(OLED_SCL_GPIO_Port, OLED_SCL_Pin);
    GPIO_SetBits(OLED_SDA_GPIO_Port, OLED_SDA_Pin);

#else
    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能相应GPIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);

    // 配置SCL引脚
    GPIO_InitStructure.GPIO_Pin = OLED_SCL_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SCL_GPIO_Port, &GPIO_InitStructure);

    // 配置SDA引脚
    GPIO_InitStructure.GPIO_Pin = OLED_SDA_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SDA_GPIO_Port, &GPIO_InitStructure);

    // 配置CS引脚
    GPIO_InitStructure.GPIO_Pin = OLED_CS_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_CS_GPIO_Port, &GPIO_InitStructure);

    // 配置DC引脚
    GPIO_InitStructure.GPIO_Pin = OLED_DC_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_DC_GPIO_Port, &GPIO_InitStructure);

    // 配置RES引脚
    GPIO_InitStructure.GPIO_Pin = OLED_RES_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_RES_GPIO_Port, &GPIO_InitStructure);

    // 设置引脚初始电平
    GPIO_SetBits(OLED_SCL_GPIO_Port, OLED_SCL_Pin);
    GPIO_SetBits(OLED_SDA_GPIO_Port, OLED_SDA_Pin);
    GPIO_SetBits(OLED_CS_GPIO_Port, OLED_CS_Pin);
    GPIO_SetBits(OLED_DC_GPIO_Port, OLED_DC_Pin);
    GPIO_SetBits(OLED_RES_GPIO_Port, OLED_RES_Pin);

    // 复位OLED
    OLED_RST_Set();
    delay_ms(100);
    OLED_RST_Clr();
    delay_ms(100);
    OLED_RST_Set();
#endif
    delay_ms(100); // 这里的延时很重要
#if 1
    for (u8 i = 0; i < sizeof(OLED_ConfigCmd) / sizeof(OLED_ConfigCmd[0]); i++)
    {
        OLED_WR_Byte(OLED_ConfigCmd[i], OLED_CMD);
    }
#else
    OLED_WR_Byte(0xAE, OLED_CMD); // display off
    OLED_WR_Byte(0x20, OLED_CMD); // Set Memory Addressing Mode
    OLED_WR_Byte(0x10, OLED_CMD); // 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
    OLED_WR_Byte(0xb0, OLED_CMD); // Set Page Start Address for Page Addressing Mode,0-7
    OLED_WR_Byte(0xc8, OLED_CMD); // Set COM Output Scan Direction
    OLED_WR_Byte(0x00, OLED_CMD); //---set low column address
    OLED_WR_Byte(0x10, OLED_CMD); //---set high column address
    OLED_WR_Byte(0x40, OLED_CMD); //--set start line address
    OLED_WR_Byte(0x81, OLED_CMD); //--set contrast control register
    OLED_WR_Byte(0xff, OLED_CMD); // 亮度调节 0x00~0xff
    OLED_WR_Byte(0xa1, OLED_CMD); //--set segment re-map 0 to 127
    OLED_WR_Byte(0xa6, OLED_CMD); //--set normal display
    OLED_WR_Byte(0xa8, OLED_CMD); //--set multiplex ratio(1 to 64)
    OLED_WR_Byte(0x3F, OLED_CMD); //
    OLED_WR_Byte(0xa4, OLED_CMD); // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    OLED_WR_Byte(0xd3, OLED_CMD); //-set display offset
    OLED_WR_Byte(0x00, OLED_CMD); //-not offset
    OLED_WR_Byte(0xd5, OLED_CMD); //--set display clock divide ratio/oscillator frequency
    OLED_WR_Byte(0xf0, OLED_CMD); //--set divide ratio
    OLED_WR_Byte(0xd9, OLED_CMD); //--set pre-charge period
    OLED_WR_Byte(0x22, OLED_CMD); //
    OLED_WR_Byte(0xda, OLED_CMD); //--set com pins hardware configuration
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xdb, OLED_CMD); //--set vcomh
    OLED_WR_Byte(0x20, OLED_CMD); // 0x20,0.77xVcc
    OLED_WR_Byte(0x8d, OLED_CMD); //--set DC-DC enable
    OLED_WR_Byte(0x14, OLED_CMD); //
    OLED_WR_Byte(0xaf, OLED_CMD); //--turn on oled panel
#endif
    OLED_Clear(0);
}
