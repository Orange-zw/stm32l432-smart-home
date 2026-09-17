/*********************************************************************
 * INCLUDES
 */
#include "board_i2c.h"
#include "delay.h"
#include "stm32f10x_gpio.h"

static void SDA_OUT_MODE(void);
static void SDA_IN_MODE(void);

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 @brief I2C驱动初始化，采用模拟IO的方式实现
 @param 无
 @return 无
*/
void IIC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(IIC_SCL_CLK | IIC_SDA_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Pin = IIC_SCL_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(IIC_SCL_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
	GPIO_Init(IIC_SDA_PORT, &GPIO_InitStructure);

	IIC_Stop();  // 给一个停止信号, 复位I2C总线上的所有设备到待机模式
}

/**
 @brief CPU发起I2C总线启动信号
 @param 无
 @return 无
*/
void IIC_Start(void)
{
	SDA_OUT_MODE();  // SDA线输出模式

	IIC_SDA_1();
	IIC_SCL_1();
	delay_ms(1);
	IIC_SDA_0();  // 当SCL高电平时，SDA出现一个下跳沿表示I2C总线启动信号
	delay_ms(1);
	IIC_SCL_0();  // 钳住I2C总线，准备发送或接收数据
}

/**
 @brief CPU发起I2C总线停止信号
 @param 无
 @return 无
*/
void IIC_Stop(void)
{
	SDA_OUT_MODE();  // SDA线输出模式

	IIC_SCL_0();
	IIC_SDA_0();
	IIC_SCL_1();
	delay_ms(1);
	IIC_SDA_1();  // 当SCL高电平时，SDA出现一个上跳沿表示I2C总线停止信号
	delay_ms(1);
}

/**
 @brief CPU向I2C总线设备发送8bit数据
 @param ucByte -[in] 等待发送的字节
 @return 无
*/
void IIC_SendByte(uint8_t ucByte)
{
	uint8_t i;

	SDA_OUT_MODE();  // SDA线输出模式

	IIC_SCL_0();  // 拉低时钟开始数据传输

	for (i = 0; i < 8; i++)
	{
		if (ucByte & 0x80)
		{
			IIC_SDA_1();
		}
		else
		{
			IIC_SDA_0();
		}
		ucByte <<= 1;
		delay_ms(1);
		IIC_SCL_1();
		delay_ms(1);
		IIC_SCL_0();
		delay_ms(1);
	}
}

/**
 @brief CPU从I2C总线设备读取8bit数据
 @param 无
 @return 读到的数据
*/
uint8_t IIC_ReadByte(void)
{
	uint8_t i = 0;
	uint8_t value = 0;

	SDA_IN_MODE();  // SDA线输入模式

	for (i = 0; i < 8; i++)
	{
		value <<= 1;
		IIC_SCL_1();
		delay_ms(1);
		if (IIC_SDA_READ())
		{
			value++;
		}
		IIC_SCL_0();
		delay_ms(1);
	}
	IIC_Ack();

	return value;
}

/**
 @brief CPU产生一个时钟，并读取器件的ACK应答信号
 @param 无
 @return 返回0表示正确应答，1表示无器件响应
*/
uint8_t IIC_WaitAck(void)
{
	uint8_t result = 0;

	SDA_IN_MODE();  // SDA线输入模式

	IIC_SDA_1();  // CPU释放SDA总线
	delay_ms(1);
	IIC_SCL_1();  // CPU驱动SCL = 1, 此时器件会返回ACK应答
	delay_ms(1);
	if (IIC_SDA_READ())
	{
		result = 1;
	}
	else
	{
		result = 0;
	}
	IIC_SCL_0();
	delay_ms(1);

	return result;
}

/**
 @brief CPU产生一个ACK信号
 @param 无
 @return 无
*/
void IIC_Ack(void)
{
	SDA_OUT_MODE();  // SDA线输出模式

	IIC_SDA_0();  // CPU驱动SDA = 0
	delay_ms(1);
	IIC_SCL_1();  // CPU产生1个时钟
	delay_ms(1);
	IIC_SCL_0();
	delay_ms(1);
	IIC_SDA_1();  // CPU释放SDA总线
}

/**
 @brief CPU产生1个NACK信号
 @param 无
 @return 无
*/
void IIC_NAck(void)
{
	SDA_OUT_MODE();  // SDA线输出模式

	IIC_SDA_1();  // CPU驱动SDA = 1
	delay_ms(1);
	IIC_SCL_1();  // CPU产生1个时钟
	delay_ms(1);
	IIC_SCL_0();
	delay_ms(1);
}

/**
 @brief 检测I2C总线设备，CPU向发送设备地址，然后读取设备应答来判断该设备是否存在
 @param address -[in] 设备的I2C总线地址+读写控制bit（0 = w， 1 = r)
 @return 0 - 表示正确， 1 - 表示未探测到
*/
uint8_t IIC_CheckDevice(uint8_t address)
{
	uint8_t ucAck;

	IIC_Init();             // 初始化I2C
	IIC_Start();            // 发送启动信号
	IIC_SendByte(address);  // 设备的I2C总线地址+读写控制bit（0 = w， 1 = r)
	ucAck = IIC_WaitAck();  // 检测设备的ACK应答
	IIC_Stop();             // 发送停止信号

	return ucAck;
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */
/**
 @brief SDA输出配置
 @param 无
 @return 无
*/
static void SDA_OUT_MODE(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(IIC_SDA_PORT, &GPIO_InitStructure);
}

/**
 @brief SDA输入配置
 @param 无
 @return 无
*/
static void SDA_IN_MODE(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(IIC_SDA_PORT, &GPIO_InitStructure);
}

/****************************************************END OF FILE****************************************************/
