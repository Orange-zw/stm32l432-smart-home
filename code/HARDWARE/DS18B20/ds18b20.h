#ifndef __DS18B20_H
#define __DS18B20_H
#include "sys.h"
#include "stdio.h"

/*更改引脚之后修改这个地方，刚才是PB0的，现在改成PB7的，只用修改这个地方就可以了*/
#define GPIO_CLOCK RCC_APB2Periph_GPIOB // 时钟
#define GPIO_WHAT GPIOB                 // GPIO组
#define GPIO_PIN GPIO_Pin_7             // GPIO脚
#define GPIO_PIN_DATA 7                 // 就是第几个
#define DS18B20_DQ_OUT PBout(7)         // 数据端口	PB7
#define DS18B20_DQ_IN PBin(7)           // 数据端口	PB7
/*更改引脚之后修改这个地方，刚才是PB0的，现在改成PB7的，只用修改这个地方就可以了*/

#if (GPIO_PIN_DATA < 8)
#define GPIO_DATA (0xFFFFFFFF & (~(0x0f << 4 * GPIO_PIN_DATA)))
#define DS18B20_IO_IN()                           \
    {                                             \
        GPIO_WHAT->CRL &= GPIO_DATA;              \
        GPIO_WHAT->CRL |= 8 << 4 * GPIO_PIN_DATA; \
    }
#define DS18B20_IO_OUT()                          \
    {                                             \
        GPIO_WHAT->CRL &= GPIO_DATA;              \
        GPIO_WHAT->CRL |= 3 << 4 * GPIO_PIN_DATA; \
    } // X乘以4=20
#endif

#if (GPIO_PIN_DATA >= 8)
#define GPIO_DATA (0xFFFFFFFF & (~(0x0f << 4 * (GPIO_PIN_DATA - 8))))
#define DS18B20_IO_IN()                                 \
    {                                                   \
        GPIO_WHAT->CRH &= GPIO_DATA;                    \
        GPIO_WHAT->CRH |= 8 << 4 * (GPIO_PIN_DATA - 8); \
    }
#define DS18B20_IO_OUT()                                \
    {                                                   \
        GPIO_WHAT->CRH &= GPIO_DATA;                    \
        GPIO_WHAT->CRH |= 3 << 4 * (GPIO_PIN_DATA - 8); \
    } // X乘以4=20
#endif

u8 DS18B20_Init(void);               // 初始化DS18B20
short DS18B20_Get_Temp(void);        // 获取温度
short DS18B20_Get_Stable_Temp(void); // 获取稳定温度，避免初始85℃读数
void DS18B20_Start(void);            // 开始温度转换
void DS18B20_Write_Byte(u8 dat);     // 写入一个字节
u8 DS18B20_Read_Byte(void);          // 读出一个字节
u8 DS18B20_Read_Bit(void);           // 读出一个位
u8 DS18B20_Check(void);              // 检测是否存在DS18B20
void DS18B20_Rst(void);              // 复位DS18B20
#endif
