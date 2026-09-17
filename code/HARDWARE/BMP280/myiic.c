#include "myiic.h"
#include "delay.h"

// ��ʼ��IIC
void IIC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	// RCC->APB2ENR|=1<<4;//��ʹ������IO PORTCʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // �������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	IIC_SCL = 1;
	IIC_SDA = 1;
}
// ����IIC��ʼ�ź�
void IIC_Start(void)
{
	SDA_OUT(); // sda�����
	IIC_SDA = 1;
	IIC_SCL = 1;
	delay_us(4);
	IIC_SDA = 0; // START:when CLK is high,DATA change form high to low
	delay_us(4);
	IIC_SCL = 0; // ǯסI2C���ߣ�׼�����ͻ��������
}
// ����IICֹͣ�ź�
void IIC_Stop(void)
{
	SDA_OUT(); // sda�����
	IIC_SCL = 0;
	IIC_SDA = 0; // STOP:when CLK is high DATA change form low to high
	delay_us(4);
	IIC_SCL = 1;
	IIC_SDA = 1; // ����I2C���߽����ź�
	delay_us(4);
}
// �ȴ�Ӧ���źŵ���
// ����ֵ��1������Ӧ��ʧ��
//         0������Ӧ��ɹ�
u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime = 0;
	SDA_IN(); // SDA����Ϊ����
	IIC_SDA = 1;
	delay_us(1);
	IIC_SCL = 1;
	delay_us(1);
	while (READ_SDA)
	{
		ucErrTime++;
		if (ucErrTime > 250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL = 0; // ʱ�����0
	return 0;
}
// ����ACKӦ��
void IIC_Ack(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 0;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(2);
	IIC_SCL = 0;
}
// ������ACKӦ��
void IIC_NAck(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 1;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(2);
	IIC_SCL = 0;
}
// IIC����һ���ֽ�
// ���شӻ�����Ӧ��
// 1����Ӧ��
// 0����Ӧ��
void IIC_Send_Byte(u8 txd)
{
	u8 t;
	SDA_OUT();
	IIC_SCL = 0; // ����ʱ�ӿ�ʼ���ݴ���
	for (t = 0; t < 8; t++)
	{
		IIC_SDA = (txd & 0x80) >> 7;
		txd <<= 1;
		delay_us(2); // ��TEA5767��������ʱ���Ǳ����
		IIC_SCL = 1;
		delay_us(2);
		IIC_SCL = 0;
		delay_us(2);
	}
}
// ��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK
u8 IIC_Read_Byte(unsigned char ack)
{
	unsigned char i, receive = 0;
	SDA_IN(); // SDA����Ϊ����
	for (i = 0; i < 8; i++)
	{
		IIC_SCL = 0;
		delay_us(2);
		IIC_SCL = 1;
		receive <<= 1;
		if (READ_SDA)
			receive++;
		delay_us(1);
	}
	if (!ack)
		IIC_NAck(); // ����nACK
	else
		IIC_Ack(); // ����ACK
	return receive;
}

// ��ָ����ַ����һ������
// ReadAddr:��ʼ�����ĵ�ַ
// ����ֵ  :����������
u8 iicDevReadByte(u8 devaddr, u8 addr)
{
	u8 temp = 0;
	IIC_Start();
	IIC_Send_Byte(devaddr); // ��������д����
	IIC_Wait_Ack();
	IIC_Send_Byte(addr); // ���͵͵�ַ
	IIC_Wait_Ack();

	IIC_Start();
	IIC_Send_Byte(devaddr | 1); // ��������������
	IIC_Wait_Ack();
	temp = IIC_Read_Byte(0);
	IIC_Stop(); // ����һ��ֹͣ����
	return temp;
}

// ����������ֽ�
// addr:��ʼ��ַ
// rbuf:�����ݻ���
// len:���ݳ���
void iicDevRead(u8 devaddr, u8 addr, u8 len, u8 *rbuf)
{
	int i = 0;
	IIC_Start();
	IIC_Send_Byte(devaddr);
	IIC_Wait_Ack();
	IIC_Send_Byte(addr); // ��ַ����
	IIC_Wait_Ack();

	IIC_Start();
	IIC_Send_Byte(devaddr | 1);
	IIC_Wait_Ack();
	for (i = 0; i < len; i++)
	{
		if (i == len - 1)
		{
			rbuf[i] = IIC_Read_Byte(0); // ���һ���ֽڲ�Ӧ��
		}
		else
			rbuf[i] = IIC_Read_Byte(1);
	}
	IIC_Stop();
}

// ��ָ����ַд��һ������
// WriteAddr :д�����ݵ�Ŀ�ĵ�ַ
// DataToWrite:Ҫд�������
void iicDevWriteByte(u8 devaddr, u8 addr, u8 data)
{
	IIC_Start();
	IIC_Send_Byte(devaddr); // ��������д����
	IIC_Wait_Ack();
	IIC_Send_Byte(addr); // ���͵͵�ַ
	IIC_Wait_Ack();
	IIC_Send_Byte(data); // �����ֽ�
	IIC_Wait_Ack();
	IIC_Stop(); // ����һ��ֹͣ����
}

// ����д����ֽ�
// addr:��ʼ��ַ
// wbuf:д���ݻ���
// len:���ݵĳ���
void iicDevWrite(u8 devaddr, u8 addr, u8 len, u8 *wbuf)
{
	int i = 0;
	IIC_Start();
	IIC_Send_Byte(devaddr);
	IIC_Wait_Ack();
	IIC_Send_Byte(addr); // ��ַ����
	IIC_Wait_Ack();
	for (i = 0; i < len; i++)
	{
		IIC_Send_Byte(wbuf[i]);
		IIC_Wait_Ack();
	}
	IIC_Stop();
}
