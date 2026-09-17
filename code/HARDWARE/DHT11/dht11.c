#include "dht11.h"
#include "stdlib.h"

// 静态设备存储（支持最多8个设备）
#define MAX_DHT11_DEVICES 8
static DHT11_Device dht11_devices[MAX_DHT11_DEVICES];
static uint8_t dht11_device_count = 0;

// 根据GPIO端口获取对应的时钟使能位
static uint32_t Get_RCC_APB2Periph(GPIO_TypeDef *GPIOx)
{
	if (GPIOx == GPIOA)
		return RCC_APB2Periph_GPIOA;
	else if (GPIOx == GPIOB)
		return RCC_APB2Periph_GPIOB;
	else if (GPIOx == GPIOC)
		return RCC_APB2Periph_GPIOC;
	else if (GPIOx == GPIOD)
		return RCC_APB2Periph_GPIOD;
	else if (GPIOx == GPIOE)
		return RCC_APB2Periph_GPIOE;
	else if (GPIOx == GPIOF)
		return RCC_APB2Periph_GPIOF;
	else if (GPIOx == GPIOG)
		return RCC_APB2Periph_GPIOG;
	return 0;
}

// 根据GPIO引脚获取引脚编号
static uint8_t Get_Pin_Num(uint16_t GPIO_Pin)
{
	uint8_t pin_num = 0;
	while (GPIO_Pin >>= 1)
		pin_num++;
	return pin_num;
}

// 根据GPIO端口获取对应的输出宏
static uint32_t Get_GPIO_Out_Addr(GPIO_TypeDef *GPIOx)
{
	if (GPIOx == GPIOA)
		return GPIOA_ODR_Addr;
	else if (GPIOx == GPIOB)
		return GPIOB_ODR_Addr;
	else if (GPIOx == GPIOC)
		return GPIOC_ODR_Addr;
	else if (GPIOx == GPIOD)
		return GPIOD_ODR_Addr;
	else if (GPIOx == GPIOE)
		return GPIOE_ODR_Addr;
	else if (GPIOx == GPIOF)
		return GPIOF_ODR_Addr;
	else if (GPIOx == GPIOG)
		return GPIOG_ODR_Addr;
	return 0;
}

// 根据GPIO端口获取对应的输入宏
static uint32_t Get_GPIO_In_Addr(GPIO_TypeDef *GPIOx)
{
	if (GPIOx == GPIOA)
		return GPIOA_IDR_Addr;
	else if (GPIOx == GPIOB)
		return GPIOB_IDR_Addr;
	else if (GPIOx == GPIOC)
		return GPIOC_IDR_Addr;
	else if (GPIOx == GPIOD)
		return GPIOD_IDR_Addr;
	else if (GPIOx == GPIOE)
		return GPIOE_IDR_Addr;
	else if (GPIOx == GPIOF)
		return GPIOF_IDR_Addr;
	else if (GPIOx == GPIOG)
		return GPIOG_IDR_Addr;
	return 0;
}

// 动态GPIO输出操作
static void DHT11_DQ_OUT_Set(DHT11_Device *dev, uint8_t value)
{
	uint32_t addr = Get_GPIO_Out_Addr(dev->GPIOx);
	if (addr)
	{
		BIT_ADDR(addr, dev->pin_num) = value;
	}
}

// 动态GPIO输入操作
static uint8_t DHT11_DQ_IN_Get(DHT11_Device *dev)
{
	uint32_t addr = Get_GPIO_In_Addr(dev->GPIOx);
	if (addr)
	{
		return BIT_ADDR(addr, dev->pin_num);
	}
	return 0;
}

// 动态设置GPIO为输入模式
static void DHT11_IO_IN(DHT11_Device *dev)
{
	if (dev->pin_num < 8)
	{
		// 低8位引脚使用CRL寄存器
		uint32_t mask = 0xFFFFFFFF & (~(0x0f << (4 * dev->pin_num)));
		dev->GPIOx->CRL &= mask;
		dev->GPIOx->CRL |= 8 << (4 * dev->pin_num);  // 输入浮空模式
	}
	else
	{
		// 高8位引脚使用CRH寄存器
		uint32_t mask = 0xFFFFFFFF & (~(0x0f << (4 * (dev->pin_num - 8))));
		dev->GPIOx->CRH &= mask;
		dev->GPIOx->CRH |= 8 << (4 * (dev->pin_num - 8));  // 输入浮空模式
	}
}

// 动态设置GPIO为输出模式
static void DHT11_IO_OUT(DHT11_Device *dev)
{
	if (dev->pin_num < 8)
	{
		// 低8位引脚使用CRL寄存器
		uint32_t mask = 0xFFFFFFFF & (~(0x0f << (4 * dev->pin_num)));
		dev->GPIOx->CRL &= mask;
		dev->GPIOx->CRL |= 3 << (4 * dev->pin_num);  // 推挽输出，50MHz
	}
	else
	{
		// 高8位引脚使用CRH寄存器
		uint32_t mask = 0xFFFFFFFF & (~(0x0f << (4 * (dev->pin_num - 8))));
		dev->GPIOx->CRH &= mask;
		dev->GPIOx->CRH |= 3 << (4 * (dev->pin_num - 8));  // 推挽输出，50MHz
	}
}

// 创建DHT11设备实例
DHT11_Device *DHT11_Create(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	if (dht11_device_count >= MAX_DHT11_DEVICES)
		return NULL;  // 设备数量已达上限

	DHT11_Device *dev = &dht11_devices[dht11_device_count++];
	dev->GPIOx = GPIOx;
	dev->GPIO_Pin = GPIO_Pin;
	dev->RCC_APB2Periph = Get_RCC_APB2Periph(GPIOx);
	dev->pin_num = Get_Pin_Num(GPIO_Pin);

	DHT11_Init(dev);

	return dev;
}

// 复位DHT11
void DHT11_Rst(DHT11_Device *dev)
{
	if (dev == NULL)
		return;
	DHT11_IO_OUT(dev);         // SET OUTPUT
	DHT11_DQ_OUT_Set(dev, 0);  // 拉低DQ
	delay_ms(20);              // 拉低至少18ms
	DHT11_DQ_OUT_Set(dev, 1);  // DQ=1
	delay_us(30);              // 主机拉高20~40us
}

// 等待DHT11的回应
// 返回1:未检测到DHT11的存在
// 返回0:存在
u8 DHT11_Check(DHT11_Device *dev)
{
	if (dev == NULL)
		return 1;

	u8 retry = 0;
	DHT11_IO_IN(dev);                            // SET INPUT
	while (DHT11_DQ_IN_Get(dev) && retry < 100)  // DHT11会拉低40~80us
	{
		retry++;
		delay_us(1);
	}
	if (retry >= 100)
		return 1;
	else
		retry = 0;
	while (!DHT11_DQ_IN_Get(dev) && retry < 100)  // DHT11拉低后会再次拉高40~80us
	{
		retry++;
		delay_us(1);
	}
	if (retry >= 100)
		return 1;
	return 0;
}

// 从DHT11读取一个位
// 返回值：1/0
u8 DHT11_Read_Bit(DHT11_Device *dev)
{
	if (dev == NULL)
		return 0;

	u8 retry = 0;
	while (DHT11_DQ_IN_Get(dev) && retry < 100)  // 等待变为低电平
	{
		retry++;
		delay_us(1);
	}
	retry = 0;
	while (!DHT11_DQ_IN_Get(dev) && retry < 100)  // 等待变高电平
	{
		retry++;
		delay_us(1);
	}
	delay_us(40);  // 等待40us
	if (DHT11_DQ_IN_Get(dev))
		return 1;
	else
		return 0;
}

// 从DHT11读取一个字节
// 返回值：读到的数据
u8 DHT11_Read_Byte(DHT11_Device *dev)
{
	if (dev == NULL)
		return 0;

	u8 i, dat;
	dat = 0;
	for (i = 0; i < 8; i++)
	{
		dat <<= 1;
		dat |= DHT11_Read_Bit(dev);
	}
	return dat;
}

// 初始化DHT11的IO口 DQ 同时检测DHT11的存在
// 返回1:不存在
// 返回0:存在
u8 DHT11_Init(DHT11_Device *dev)
{
	if (dev == NULL)
		return 1;

	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(dev->RCC_APB2Periph, ENABLE);  // 使能对应端口时钟

	GPIO_InitStructure.GPIO_Pin = dev->GPIO_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(dev->GPIOx, &GPIO_InitStructure);
	GPIO_SetBits(dev->GPIOx, dev->GPIO_Pin);  // 拉高引脚
	DHT11_Rst(dev);                           // 复位DHT11
	// return DHT11_Check(dev);                  // 等待DHT11的回应
	return 0;
}

uint8_t Read_DHT11(DHT11_Device *dev, DHT11_Data_t *DHT11_Data)
{
	if (dev == NULL || DHT11_Data == NULL)
		return ERROR;

	u8 buf[5];
	u8 i;
	DHT11_Data_t temp_data;
	DHT11_Rst(dev);
	if (DHT11_Check(dev) == 0)
	{
		for (i = 0; i < 5; i++)  // 读取40位数据
		{
			buf[i] = DHT11_Read_Byte(dev);
		}
		if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
		{
			temp_data.humi_int = buf[0];
			temp_data.humi_deci = buf[1];
			temp_data.temp_int = buf[2];
			temp_data.temp_deci = buf[3];
			temp_data.check_sum = buf[4];
		}
	}
	else
		return ERROR;
	/*检查读取的数据是否正确*/
	if ((temp_data.check_sum == temp_data.humi_int + temp_data.humi_deci + temp_data.temp_int + temp_data.temp_deci) &&
	    temp_data.humi_int >= 0 && temp_data.humi_int <= 100 &&
	    temp_data.temp_int >= 0 && temp_data.temp_int <= 50 &&
	    temp_data.humi_deci >= 0 && temp_data.humi_deci <= 100 &&
	    temp_data.temp_deci >= 0 && temp_data.temp_deci <= 100)
	{
		*DHT11_Data = temp_data;
		return SUCCESS;
	}
	else
	{
		return ERROR;
	}
}

/*************************************END OF FILE******************************/
