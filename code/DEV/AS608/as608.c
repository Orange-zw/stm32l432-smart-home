#include "as608.h"
#include <string.h>
#include "DI.h"
#include "IO_sensor.h"
#include "ring_buf.h"

static USART_handle_t as608_usart = NULL;
static DI_handle_t finger_sensor = NULL;
static ringbuf_t *rx_ringbuf;
static Packet recvPacket;

static void MYUSART_SendData(uint8_t *data, uint16_t len)
{
	USART_OP(as608_usart, send, data, len);
}
static int MYUSART_ReceiveData(uint8_t *data, uint16_t len, uint16_t timeout)
{
	timeout = timeout / 10;
	while (ringbuf_get_element_count(rx_ringbuf) < len && timeout > 0)
	{
		timeout--;
		delay_ms(10);
	}
	if (timeout == 0)
	{
		return -1;
	}
	ringbuf_read_elements(rx_ringbuf, data, len);
}

static void rx_callback(uint8_t *data, uint16_t length)
{
	ringbuf_write_elements(rx_ringbuf, data, length);
}

// 发送包头
static void SendHead(void)
{
	// MYUSART_SendData(0xEF, 1);
	// MYUSART_SendData(0x01, 1);
	MYUSART_SendData((uint8_t[]){0xEF, 0x01}, 2);
}
// 发送地址
static void SendAddr(void)
{
	MYUSART_SendData((uint8_t[]){REG_ADDR >> 24, REG_ADDR >> 16, REG_ADDR >> 8, REG_ADDR}, 4);
}
// 发送包标识,
static void SendFlag(uint8_t flag)
{
	MYUSART_SendData(&flag, 1);
}
// 发送包长度
static void SendLength(int length)
{
	MYUSART_SendData((uint8_t[]){length >> 8, length}, 2);
}
// 发送指令码
static void Sendcmd(uint8_t cmd)
{
	MYUSART_SendData(&cmd, 1);
}
// 发送校验和
static void SendCheck(uint16_t check)
{
	MYUSART_SendData((uint8_t[]){check >> 8}, 1);
	MYUSART_SendData((uint8_t[]){check}, 1);
}
// 判断中断接收的数组有没有应答包
// waittime为等待中断接收数据的时间（单位1ms）
// 返回值：数据包首地址
static uint8_t *JudgeStr(uint16_t waittime)
{
	// char *data;
	// uint8_t str[8];
	// str[0] = 0xef;
	// str[1] = 0x01;
	// str[2] = REG_ADDR >> 24;
	// str[3] = REG_ADDR >> 16;
	// str[4] = REG_ADDR >> 8;
	// str[5] = REG_ADDR;
	// str[6] = 0x07;
	// str[7] = '\0';
	// USART2_RX_STA = 0;
	// while (--waittime)
	// {
	// 	delay_ms(1);
	// 	if (USART2_RX_STA & 0X8000) // 接收到一次数据
	// 	{
	// 		USART2_RX_STA = 0;
	// 		data = strstr((const char *)USART2_RX_BUF, (const char *)str);
	// 		if (data)
	// 			return (uint8_t *)data;
	// 	}
	// }

	MYUSART_ReceiveData((uint8_t *)&recvPacket, sizeof(PacketHeader), waittime);
	if (recvPacket.header.head == __rev16(PACKET_HEAD) &&
	    recvPacket.header.addr == __rev32(REG_ADDR) &&
	    recvPacket.header.flag == 0x07)
	{
		return (uint8_t *)&recvPacket;
	}

	return 0;
}

uint16_t CalCheckSum(uint8_t *data, uint16_t len)
{
	uint16_t sum = 0;
	for (uint16_t i = 0; i < len; i++)
	{
		sum += data[i];
	}
	return sum;
}

void SendPacket(uint8_t flag, uint8_t cmd, uint8_t *data, uint16_t dataLen)
{
	Packet packet = {0};
	packet.header.head = __rev16(PACKET_HEAD);
	packet.header.addr = __rev32(REG_ADDR);
	packet.header.flag = flag;
	// 包长度至校验和（指令、参数或数据）的总字节数，包含校验和，但不包含包长度本身的字节数。
	packet.header.length = __rev16(dataLen + 2 + 1);  // 数据长度 + cmd(1byte) + checksum(2bytes)
	packet.header.cmd = cmd;
	if (dataLen > 0 && data != NULL)
	{
		memcpy(packet.data, data, dataLen);
	}
	// 校验和是从包标识至校验和之间所有字节之和，超出 2 字节的进位忽略
	// flag(1byte) + length(2bytes) + cmd(1byte) + data(dataLen bytes)
	packet.checksum = CalCheckSum((uint8_t *)&packet.header.flag, 1 + 2 + 1 + dataLen);
	// 将校验和附加到数据包末尾
	packet.data[dataLen] = (packet.checksum >> 8) & 0xFF;
	packet.data[dataLen + 1] = packet.checksum & 0xFF;

	// 发送数据包
	// SendData((uint8_t *)&packet, sizeof(PacketHeader) + dataLen + 2);
	MYUSART_SendData((uint8_t *)&packet, sizeof(PacketHeader) + dataLen + 2);
}

void PS_Init(USART_TypeDef *instance)
{
	as608_usart = USART_Create(instance, 56000);
	if (as608_usart == NULL)
	{
		return;
	}
	USART_OP(as608_usart, set_rx_idle_callback, rx_callback);
	rx_ringbuf = ringbuf_create(128, sizeof(uint8_t));

	delay_ms(100);
	PS_HandShake();
}

void PS_Touch_Init(GPIO_TypeDef *gpio_port, uint16_t gpio_pin)
{
	finger_sensor = DI_Create(gpio_port, gpio_pin, IO_HIGH);
}

bool PS_IsTouch(void)
{
	return DI_OP(finger_sensor, is_active);
}

// static void (*touch_callback)(int id, void *args) = NULL;
// static void touch_sensor_callback_impl(DISensor *sensor, void *args)
// {
// 	int id = PS_AutoIdentify();
// 	touch_callback(id, args);
// }
// void PS_setTouchCallback(void (*callback)(int id, void *args), void *args)
// {
// 	touch_callback = callback;
// 	DI_SET_CALLBACK(finger_sensor, DISENSOR_EVENT_RISING, touch_sensor_callback_impl, args);
// }

// 录入图像 PS_GetImage
// 功能:探测手指，探测到后录入指纹图像存于ImageBuffer。
// 模块返回确认字
uint8_t PS_GetImage(void)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x03);
	Sendcmd(0x01);
	temp = 0x01 + 0x03 + 0x01;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
		ensure = data[9];
	else
		ensure = 0xff;
	return ensure;
}
// 生成特征 PS_GenChar
// 功能:将ImageBuffer中的原始图像生成指纹特征文件存于CharBuffer1或CharBuffer2
// 参数:BufferID --> charBuffer1:0x01	charBuffer1:0x02
// 模块返回确认字
uint8_t PS_GenChar(uint8_t BufferID)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x04);
	Sendcmd(0x02);
	MYUSART_SendData(&BufferID, 1);
	temp = 0x01 + 0x04 + 0x02 + BufferID;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
		ensure = data[9];
	else
		ensure = 0xff;
	return ensure;
}
// 精确比对两枚指纹特征 PS_Match
// 功能:精确比对CharBuffer1 与CharBuffer2 中的特征文件
// 模块返回确认字
uint8_t PS_Match(void)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x03);
	Sendcmd(0x03);
	temp = 0x01 + 0x03 + 0x03;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
		ensure = data[9];
	else
		ensure = 0xff;
	return ensure;
}
// 搜索指纹 PS_Search
// 功能:以CharBuffer1或CharBuffer2中的特征文件搜索整个或部分指纹库.若搜索到，则返回页码。
// 参数:  BufferID @ref CharBuffer1	CharBuffer2
// 说明:  模块返回确认字，页码（相配指纹模板）
uint8_t PS_Search(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x08);
	Sendcmd(0x04);
	MYUSART_SendData(&BufferID, 1);
	MYUSART_SendData((uint8_t[]){StartPage >> 8, (uint8_t)StartPage}, 2);
	MYUSART_SendData((uint8_t[]){PageNum >> 8, (uint8_t)PageNum}, 2);
	temp = 0x01 + 0x08 + 0x04 + BufferID + (StartPage >> 8) + (uint8_t)StartPage + (PageNum >> 8) + (uint8_t)PageNum;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
	{
		ensure = data[9];
		p->pageID = (data[10] << 8) + data[11];
		p->mathscore = (data[12] << 8) + data[13];
	}
	else
		ensure = 0xff;
	return ensure;
}

uint8_t PS_Sleep(void)  // 6.4.6 休眠指令
{
	SendPacket(0x01, 0x32, NULL, 0);
	delay_ms(100);
	MYUSART_ReceiveData((uint8_t *)&recvPacket, sizeof(PacketHeader) + 2, 2000);
	if (recvPacket.header.head == __rev16(PACKET_HEAD) &&
	    recvPacket.header.addr == __rev32(REG_ADDR) &&
	    recvPacket.header.cmd == 0x00)
		return 0;
}

uint8_t PS_Cancel(void)  // 6.4.5 取消指令
{
	SendPacket(0x01, 0x30, NULL, 0);
	delay_ms(100);
	MYUSART_ReceiveData((uint8_t *)&recvPacket, sizeof(PacketHeader) + 2, 2000);
	if (recvPacket.header.head == __rev16(PACKET_HEAD) &&
	    recvPacket.header.addr == __rev32(REG_ADDR) &&
	    recvPacket.header.cmd == 0x00)
	{
		return 1;
	}

	return 0;
}
// 合并特征（生成模板）PS_RegModel
// 功能:将CharBuffer1与CharBuffer2中的特征文件合并生成 模板,结果存于CharBuffer1与CharBuffer2
// 说明:  模块返回确认字
uint8_t PS_RegModel(void)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x03);
	Sendcmd(0x05);
	temp = 0x01 + 0x03 + 0x05;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
		ensure = data[9];
	else
		ensure = 0xff;
	return ensure;
}
// 储存模板 PS_StoreChar
// 功能:将 CharBuffer1 或 CharBuffer2 中的模板文件存到 PageID 号flash数据库位置。
// 参数:  BufferID @ref charBuffer1:0x01	charBuffer1:0x02
//        PageID（指纹库位置号）
// 说明:  模块返回确认字
uint8_t PS_StoreChar(uint8_t BufferID, uint16_t PageID)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x06);
	Sendcmd(0x06);
	MYUSART_SendData(&BufferID, 1);
	MYUSART_SendData((uint8_t[]){PageID >> 8, (uint8_t)PageID}, 2);
	temp = 0x01 + 0x06 + 0x06 + BufferID + (PageID >> 8) + (uint8_t)PageID;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
		ensure = data[9];
	else
		ensure = 0xff;
	return ensure;
}

void PS_AutoEnroll(uint16_t PageID, uint8_t input_time)  // 自动注册指纹  按N次注册
{
	// uint16_t temp;
	// uint8_t input_time = 4; // 录入次数
	// SendHead();
	// SendAddr();
	// SendFlag(0x01); // 命令包标识
	// SendLength(0x0008);
	// Sendcmd(0x31);
	// MYUSART_SendData(PageID >> 8);
	// MYUSART_SendData(PageID);
	// MYUSART_SendData(input_time); // 按N次
	// MYUSART_SendData(0);
	// // MYUSART_SendData(0x10);	//要求返回模块状态，不允许覆盖ID号，不允许重复注册，要求手指离开采集
	// MYUSART_SendData(0x08); // 要求返回模块状态，允许覆盖ID号，允许重复注册，要求手指离开采集
	// temp = 0x01 + 0x0008 + 0x31 + (PageID >> 8) + (uint8_t)PageID + input_time + 0x08;
	// SendCheck(temp);
	SendPacket(0x01, 0x31, (uint8_t[]){(uint8_t)(PageID >> 8), (uint8_t)PageID, input_time, 0x00, 0x08}, 5);
}
uint16_t PS_AutoIdentify(void)  // 自动验证指纹 返回匹配到的指纹ID，未匹配返回0
{
	// uint16_t temp;

	// SendHead();
	// SendAddr();
	// SendFlag(0x01);		 // 命令包标识
	// SendLength(0x0008);	 // 长度
	// Sendcmd(0x32);		 // 指令码
	// MYUSART_SendData(1); // 安全等级 比较低
	// MYUSART_SendData(0XFF);
	// MYUSART_SendData(0XFF); // 1:N搜索 发送0XFF 0XFF
	// MYUSART_SendData(0);	// 参数1
	// MYUSART_SendData(0X04); // 参数2 不返回状态信息
	// temp = 0x01 + 0x0008 + 0x32 + 1 + 0xff + 0xff + 0x04;
	// SendCheck(temp);
	Packet recvPacket;
	uint16_t wait_time = 500;
	SendPacket(0x01, 0x32, (uint8_t[]){1, 0xFF, 0xFF, 0, 0x04}, 5);

	MYUSART_ReceiveData((uint8_t *)&recvPacket, sizeof(PacketHeader) + 7, 2000);

	if (recvPacket.header.cmd == 0x00 && recvPacket.data[0] == 0x05)
	{
		return recvPacket.data[1] << 8 | recvPacket.data[2];
	}

	return 0xffff;
}
// 删除模板 PS_DeleteChar
// 功能:  删除flash数据库中指定ID号开始的N个指纹模板
// 参数:  PageID(指纹库模板号)，N删除的模板个数。
// 说明:  模块返回确认字
uint8_t PS_DeleteChar(uint16_t PageID, uint16_t N)
{
	// uint16_t temp;
	// uint8_t ensure;
	// uint8_t *data;
	// SendHead();
	// SendAddr();
	// SendFlag(0x01); // 命令包标识
	// SendLength(0x07);
	// Sendcmd(0x0C);
	// MYUSART_SendData(PageID >> 8);
	// MYUSART_SendData(PageID);
	// MYUSART_SendData(N >> 8);
	// MYUSART_SendData(N);
	// temp = 0x01 + 0x07 + 0x0C + (PageID >> 8) + (uint8_t)PageID + (N >> 8) + (uint8_t)N;
	// SendCheck(temp);
	// data = JudgeStr(2000);
	// if (data)
	// 	ensure = data[9];
	// else
	// 	ensure = 0xff;
	// return ensure;
	SendPacket(0x01, 0x0C, (uint8_t[]){(uint8_t)(PageID >> 8), (uint8_t)PageID, (uint8_t)(N >> 8), (uint8_t)N}, 4);
	MYUSART_ReceiveData((uint8_t *)&recvPacket, sizeof(PacketHeader) + 2, 2000);
	return recvPacket.header.cmd == 0x00;
}

uint8_t PS_GetEnrollCount()  // 获取注册进度
{
	Packet recvPacket;
	uint16_t size = sizeof(PacketHeader) + 2 + 2;  // 数据包头 + 数据(1byte) + 校验和(2bytes)
	static uint16_t cnt = 0;

	for (int i = 0; i < 10; i++)
	{
		if (MYUSART_ReceiveData((uint8_t *)&recvPacket, size, 10000) == -1)
		{
			return 0xff;
		}

		if (recvPacket.header.head == __rev16(PACKET_HEAD) &&
		    recvPacket.header.addr == __rev32(REG_ADDR) &&
		    recvPacket.header.cmd == 0x00 &&
		    recvPacket.data[0] == 0x03)  // 注册指纹状态返回
		{
			cnt = recvPacket.data[1];
			return recvPacket.data[1];
		}
		else if (recvPacket.header.head == __rev16(PACKET_HEAD) &&
		         recvPacket.header.addr == __rev32(REG_ADDR) &&
		         recvPacket.header.cmd == 0x00 &&
		         recvPacket.data[0] == 0x06 &&
		         recvPacket.data[1] == 0xF2)  // 自动注册指纹状态返回
		{
			return cnt + 1;
		}
	}
	return 0xff;
}

// 清空指纹库 PS_Empty
// 功能:  删除flash数据库中所有指纹模板
// 参数:  无
// 说明:  模块返回确认字
uint8_t PS_Empty(void)
{
	// uint16_t temp;
	// uint8_t ensure;
	// uint8_t *data;
	// SendHead();
	// SendAddr();
	// SendFlag(0x01); // 命令包标识
	// SendLength(0x03);
	// Sendcmd(0x0D);
	// temp = 0x01 + 0x03 + 0x0D;
	// SendCheck(temp);
	// data = JudgeStr(2000);
	// if (data)
	// 	ensure = data[9];
	// else
	// 	ensure = 0xff;
	// return ensure;
	SendPacket(0x01, 0x0D, NULL, 0);
	MYUSART_ReceiveData((uint8_t *)&recvPacket, sizeof(PacketHeader) + 2, 2000);
	return recvPacket.header.cmd == 0x00;
}

// 写系统寄存器 PS_WriteReg
// 功能:  写模块寄存器
// 参数:  寄存器序号RegNum:4\5\6
// 说明:  模块返回确认字
uint8_t PS_WriteReg(uint8_t RegNum, uint8_t DATA)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x05);
	Sendcmd(0x0E);
	MYUSART_SendData(&RegNum, 1);
	MYUSART_SendData(&DATA, 1);
	temp = RegNum + DATA + 0x01 + 0x05 + 0x0E;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
		ensure = data[9];
	else
		ensure = 0xff;
	if (ensure == 0)
		printf("\r\n设置参数成功！");
	else
		printf("\r\n%s", EnsureMessage(ensure));
	return ensure;
}
// 读系统基本参数 PS_ReadSysPara
// 功能:  读取模块的基本参数（波特率，包大小等)
// 参数:  无
// 说明:  模块返回确认字 + 基本参数（16bytes）
uint8_t PS_ReadSysPara(SysPara *p)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x03);
	Sendcmd(0x0F);
	temp = 0x01 + 0x03 + 0x0F;
	SendCheck(temp);
	data = JudgeStr(1000);
	if (data)
	{
		ensure = data[9];
		p->PS_max = (data[14] << 8) + data[15];
		p->PS_level = data[17];
		p->PS_addr = (data[18] << 24) + (data[19] << 16) + (data[20] << 8) + data[21];
		p->PS_size = data[23];
		p->PS_N = data[25];
	}
	else
		ensure = 0xff;
	if (ensure == 0x00)
	{
		printf("\r\n模块最大指纹容量=%d", p->PS_max);
		printf("\r\n对比等级=%d", p->PS_level);
		printf("\r\n地址=%x", p->PS_addr);
		printf("\r\n波特率=%d", p->PS_N * 9600);
	}
	else
		printf("\r\n%s", EnsureMessage(ensure));
	return ensure;
}
// 设置模块地址 PS_SetAddr
// 功能:  设置模块地址
// 参数:  PS_addr
// 说明:  模块返回确认字
uint8_t PS_SetAddr(uint32_t PS_addr)
{
	// uint16_t temp;
	// uint8_t ensure;
	// uint8_t *data;
	// SendHead();
	// SendAddr();
	// SendFlag(0x01); // 命令包标识
	// SendLength(0x07);
	// Sendcmd(0x15);
	// MYUSART_SendData(PS_addr >> 24);
	// MYUSART_SendData(PS_addr >> 16);
	// MYUSART_SendData(PS_addr >> 8);
	// MYUSART_SendData(PS_addr);
	// temp = 0x01 + 0x07 + 0x15 + (uint8_t)(PS_addr >> 24) + (uint8_t)(PS_addr >> 16) + (uint8_t)(PS_addr >> 8) + (uint8_t)PS_addr;
	// SendCheck(temp);
	// REG_ADDR = PS_addr; // 发送完指令，更换地址
	// data = JudgeStr(2000);
	// if (data)
	// 	ensure = data[9];
	// else
	// 	ensure = 0xff;
	// REG_ADDR = PS_addr;
	// if (ensure == 0x00)
	// 	printf("\r\n设置地址成功！");
	// else
	// 	printf("\r\n%s", EnsureMessage(ensure));
	// return ensure;
}
// 功能： 模块内部为用户开辟了256bytes的FLASH空间用于存用户记事本,
//	该记事本逻辑上被分成 16 个页。
// 参数:  NotePageNum(0~15),Byte32(要写入内容，32个字节)
// 说明:  模块返回确认字
uint8_t PS_WriteNotepad(uint8_t NotePageNum, uint8_t *Byte32)
{
	uint16_t temp;
	uint8_t ensure, i;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(36);
	Sendcmd(0x18);
	MYUSART_SendData(&NotePageNum, 1);
	for (i = 0; i < 32; i++)
	{
		MYUSART_SendData(&Byte32[i], 1);
		temp += Byte32[i];
	}
	temp = 0x01 + 36 + 0x18 + NotePageNum + temp;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
		ensure = data[9];
	else
		ensure = 0xff;
	return ensure;
}
// 读记事PS_ReadNotepad
// 功能：  读取FLASH用户区的128bytes数据
// 参数:  NotePageNum(0~15)
// 说明:  模块返回确认字+用户信息
uint8_t PS_ReadNotepad(uint8_t NotePageNum, uint8_t *Byte32)
{
	uint16_t temp;
	uint8_t ensure, i;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x04);
	Sendcmd(0x19);
	MYUSART_SendData(&NotePageNum, 1);
	temp = 0x01 + 0x04 + 0x19 + NotePageNum;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
	{
		ensure = data[9];
		for (i = 0; i < 32; i++)
		{
			Byte32[i] = data[10 + i];
		}
	}
	else
		ensure = 0xff;
	return ensure;
}
// 高速搜索PS_HighSpeedSearch
// 功能：以 CharBuffer1或CharBuffer2中的特征文件高速搜索整个或部分指纹库。
//		  若搜索到，则返回页码,该指令对于的确存在于指纹库中 ，且登录时质量
//		  很好的指纹，会很快给出搜索结果。
// 参数:  BufferID， StartPage(起始页)，PageNum（页数）
// 说明:  模块返回确认字+页码（相配指纹模板）
uint8_t PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, uint8_t *finger_n)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x08);
	Sendcmd(0x1b);
	MYUSART_SendData((uint8_t[]){BufferID, StartPage >> 8, (uint8_t)StartPage, PageNum >> 8, (uint8_t)PageNum}, 5);

	temp = 0x01 + 0x08 + 0x1b + BufferID + (StartPage >> 8) + (uint8_t)StartPage + (PageNum >> 8) + (uint8_t)PageNum;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
	{
		ensure = data[9];
		*finger_n = data[11];
		// p->pageID 	=(data[10]<<8) +data[11];
		// p->mathscore=(data[12]<<8) +data[13];
	}
	else
		ensure = 0xff;
	return ensure;
}
// 读有效模板个数 PS_ValidTempleteNum
// 功能：读有效模板个数
// 参数: 无
// 说明: 模块返回确认字+有效模板个数ValidN
uint8_t PS_ValidTempleteNum(uint8_t *ValidN)
{
	uint16_t temp;
	uint8_t ensure;
	uint8_t *data;
	SendHead();
	SendAddr();
	SendFlag(0x01);  // 命令包标识
	SendLength(0x03);
	Sendcmd(0x1d);
	temp = 0x01 + 0x03 + 0x1d;
	SendCheck(temp);
	data = JudgeStr(2000);
	if (data)
	{
		ensure = data[9];
		// ValidN = (data[10]<<8) +data[11];
		*ValidN = data[11];
	}
	else
		ensure = 0xff;
	return ensure;
}
// 与AS608握手 PS_HandShake
uint8_t PS_HandShake(void)
{
	SendPacket(0x01, 0x35, NULL, 0);
	delay_ms(100);
	MYUSART_ReceiveData((uint8_t *)&recvPacket, sizeof(PacketHeader) + 2, 5000);  // 数据包头  + 校验和(2bytes)
	if (recvPacket.header.head == __rev16(PACKET_HEAD) &&
	    recvPacket.header.addr == __rev32(REG_ADDR) &&
	    recvPacket.header.cmd == 0x00)
	{

		return 1;
	}
	return 0;
}
// 模块应答包确认码信息解析
// 功能：解析确认码错误信息返回信息
// 参数: ensure
const char *EnsureMessage(uint8_t ensure)
{
	const char *p;
	switch (ensure)
	{
	case 0x00:
		p = "OK";
		break;
	case 0x01:
		p = "数据包接收错误";
		break;
	case 0x02:
		p = "传感器上没有手指";
		break;
	case 0x03:
		p = "录入指纹图像失败";
		break;
	case 0x04:
		p = "指纹图像太干、太淡而生不成特征";
		break;
	case 0x05:
		p = "指纹图像太湿、太糊而生不成特征";
		break;
	case 0x06:
		p = "指纹图像太乱而生不成特征";
		break;
	case 0x07:
		p = "指纹图像正常，但特征点太少（或面积太小）而生不成特征";
		break;
	case 0x08:
		p = "指纹不匹配";
		break;
	case 0x09:
		p = "没搜索到指纹";
		break;
	case 0x0a:
		p = "特征合并失败";
		break;
	case 0x0b:
		p = "访问指纹库时地址序号超出指纹库范围";
	case 0x10:
		p = "删除模板失败";
		break;
	case 0x11:
		p = "清空指纹库失败";
		break;
	case 0x15:
		p = "缓冲区内没有有效原始图而生不成图像";
		break;
	case 0x18:
		p = "读写 FLASH 出错";
		break;
	case 0x19:
		p = "未定义错误";
		break;
	case 0x1a:
		p = "无效寄存器号";
		break;
	case 0x1b:
		p = "寄存器设定内容错误";
		break;
	case 0x1c:
		p = "记事本页码指定错误";
		break;
	case 0x1f:
		p = "指纹库满";
		break;
	case 0x20:
		p = "地址错误";
		break;
	default:
		p = "模块返回确认码有误";
		break;
	}
	return p;
}
