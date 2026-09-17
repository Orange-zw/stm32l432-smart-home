#include "stm32f10x.h" // Device header
#include "R60ABD1.h"
uint8_t breathe_data = 0x00;
uint8_t heart_data = 0x00;
int flag = 0;
int flag1 = 0;

void R60ABD1_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); // 开启USART1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 开启GPIOA的时钟

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure); // 将PA9引脚初始化为复用推挽输出

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure); // 将PA10引脚初始化为上拉输入

	/*USART初始化*/
	USART_InitTypeDef USART_InitStructure;											// 定义结构体变量
	USART_InitStructure.USART_BaudRate = 115200;									// 波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 硬件流控制，不需要
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;					// 模式，发送模式和接收模式均选择
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 奇偶校验，不需要
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 停止位，选择1位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 字长，选择8位
	USART_Init(USART1, &USART_InitStructure);										// 将结构体变量交给USART_Init，配置USART1

	/*中断输出配置*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 开启串口接收数据的中断

	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 配置NVIC为分组2

	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;					  // 定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		  // 选择配置NVIC的USART1线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // 指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		  // 指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);							  // 将结构体变量交给NVIC_Init，配置NVIC外设

	/*USART使能*/
	USART_Cmd(USART1, ENABLE); // 使能USART1，串口开始运行
}

void USART1_IRQHandler(void)
{
	u8 com_data;
	static u8 RxCounter1 = 0;
	static u8 RxBuffer1[256] = {0};
	static u8 RxState = 0;
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET) // 判断是否是USART1的接收事件触发的中断
	{
		com_data = USART_ReceiveData(USART1); // 读取数据寄存器，存放在接收的数据变量
		if (RxState == 0 && com_data == 0x53) // 0x53帧头
		{

			RxState = 1;
			RxBuffer1[RxCounter1++] = com_data;
		}

		else if (RxState == 1 && com_data == 0x59) // 0x59帧头
		{
			RxState = 2;
			RxBuffer1[RxCounter1++] = com_data;
		}

		else if (RxState == 2)
		{
			RxBuffer1[RxCounter1++] = com_data;
			// 判断接收到的数据长度，假设数据长度为Lenth_H和Lenth_L
			if (RxCounter1 >= 6) // 前面已经接收了帧头和控制字等信息
			{
				uint16_t dataLength = (RxBuffer1[4] << 8) | RxBuffer1[5]; // 获得数据长度

				if (RxCounter1 >= 6 + dataLength + 3) // 检查是否接收了完整帧
				{
					if (com_data == 0x43 && RxBuffer1[RxCounter1 - 2] == 0x54) // 校验帧尾
					{
						uint8_t checksum = 0;
						for (uint8_t i = 0; i < RxCounter1 - 3; i++) // 计算校验和
						{
							checksum += RxBuffer1[i];
						}
						checksum &= 0xFF; // 取后八位

						if (checksum == RxBuffer1[RxCounter1 - 3]) // 校验和匹配
						{
							// 处理接收到的数据
							// 比如可以将数据提取出来存储到变量中
							Parse_R60ABD1data(RxBuffer1, dataLength); // 传入数据和长度
						}
					}

					// 处理完数据后，复位状态机
					RxCounter1 = 0;
					RxState = 0;
				}
			}
		}
		else // 接收异常
		{
			RxState = 0;
			RxCounter1 = 0;
			memset(RxBuffer1, 0, sizeof(RxBuffer1)); // 清空缓存
		}

		USART_ClearITPendingBit(USART1, USART_IT_RXNE); // 清除USART1的RXNE标志位
														// 读取数据寄存器会自动清除此标志位
														// 如果已经读取了数据寄存器，也可以不执行此代码
	}
}

// 处理数据函数
void Parse_R60ABD1data(uint8_t *data, uint16_t length)
{
	// 处理呼吸数据
	if (data[2] == 0x81 && data[3] == 0x02)
	{
		breathe_data = data[6];
		flag = 1;
	}
	// 处理心率数据
	else if (data[2] == 0x85 && data[3] == 0x02)
	{
		heart_data = data[6];
		flag1 = 1;
	}
}

// 获取 breathe_data 的函数
uint8_t get_hex_breathe_data(void)
{
	return breathe_data; // 返回当前的呼吸数据值
}

// 获取 heart_data 的函数
uint8_t get_hex_heart_data(void)
{
	return heart_data; // 返回当前的心率数据值
}
