#include "rc522.h"
#include "rc522Conf.h"
#include "delay.h"
#include "string.h"

static void pcdAntennaOn(void); // 开启天线
static void pcdAntennaOff(void); // 关闭天线，每次启动或者关闭天线发射之间必须有1ms间隔
static char configWorkISOType(u8 type); // 配置工作模式为ISO14443A
static void rc522GpioInit(void); // 使用模拟SPI协议，模式3，空闲状态CLK为高电平，CS为低电平开始通信
static void spiSendByte(uint8_t data); // SPI协议发送1字节数据
static uint8_t spiReadByte(void); // SPI协议读取1字节数据
static void writeRawRC(uint8_t addr, uint8_t data); // 写寄存器
static uint8_t readRawRC(uint8_t addr); // 读寄存器
static void setBitMask(u8 reg, u8 mask);
static void clearBitMask(u8 reg, u8 mask);
static char pcdComMF522(u8 ucCommand, u8* pInData, u8 ucInLenByte, u8* pOutData, u32* pOutLenBit);
static void rc522Reset(void); // 复位RC522
static void calulateCRC(u8* pIn, u8 len,u8* pOut); // 计算CRC校验码

//密码a和b
u8 KEY_A[6]= {0xff,0xff,0xff,0xff,0xff,0xff};
u8 KEY_B[6]= {0xff,0xff,0xff,0xff,0xff,0xff};

//存储块数据
unsigned char DATA0[16]= {0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char DATA1[16]= {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


void rc522Init(void)
{
    // 初始化使用的GPIO口
    rc522GpioInit();

    // 复位RC522
    rc522Reset();

    pcdAntennaOff();  //关闭天线
    delay_ms(2);             //延时2毫秒
    pcdAntennaOn();    //开启天线

    // 设置工作模式
    configWorkISOType('A');
}

static void rc522Reset(void)
{
    RSTHIGH;
    delay_us(5);
    RSTLOW;
    delay_us(5);
    RSTHIGH;
    delay_us(5);

    writeRawRC(CommandReg,PCD_RESETPHASE);  //写RC632寄存器，复位
    while(readRawRC(CommandReg) & 0x10);

    writeRawRC(ModeReg, 0x3D);            //定义发送和接收常用模式 和Mifare卡通讯，CRC初始值0x6363
    writeRawRC(TReloadRegL, 30);          //16位定时器低位    
    writeRawRC(TReloadRegH, 0);			 //16位定时器高位
    writeRawRC(TModeReg, 0x8D);			 //定义内部定时器的设置
    writeRawRC(TPrescalerReg, 0x3E);		 //设置定时器分频系数
    writeRawRC(TxAutoReg, 0x40);			 //调制发送信号为100%ASK
}

static char configWorkISOType(u8 type)
{
    if(type=='A')                        //ISO14443_A
    { 
        clearBitMask(Status2Reg,0x08);     //清RC522寄存器位
        writeRawRC(ModeReg,0x3D);          //3F//CRC初始值0x6363
        writeRawRC(RxSelReg,0x86);         //84
        writeRawRC(RFCfgReg,0x7F);         //4F  //调整卡的感应距离//RxGain = 48dB调节卡感应距离  
        writeRawRC(TReloadRegL,30);        //tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
        writeRawRC(TReloadRegH,0);
        writeRawRC(TModeReg,0x8D);
        writeRawRC(TPrescalerReg,0x3E);
        delay_us(20);
        pcdAntennaOn();    //开启天线 
    }
    else
        return 1;       //失败，返回1
   return MI_OK;        //成功返回0
}

static void rc522GpioInit(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = RST_PIN;
    GPIO_Init(RST_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = MOSI_PIN;
    GPIO_Init(MOSI_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = SCK_PIN;
    GPIO_Init(SCK_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = SDA_PIN;
    GPIO_Init(SDA_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 数据接收引脚
    GPIO_InitStructure.GPIO_Pin = MISO_PIN;
    GPIO_Init(MISO_PORT, &GPIO_InitStructure);

    // 初始化引脚电平,SPI空闲状态
    SCKHIGH;
    SDAHIGH;
}

static void spiSendByte(uint8_t data)
{
    for(u8 i=0;i<8;i++)
    {
        SCKLOW; // 准备数据
        if(data & 0x80)
            MOSIHIGH;
        else
            MOSILOW;
        SCKHIGH; // 将数据发送出去
        data <<= 1;
    }

    SCKHIGH; // 空闲状态
}

static uint8_t spiReadByte(void)
{
    uint8_t data = 0x00;

    for(u8 i=0;i<8;i++)
    {
        data <<= 1;
        SCKLOW; // 等待从机准备数据
        SCKHIGH; // 获取从机交换过来的数据
        if(MISOVALUE)
            data |= 0x01;
    }

    SCKHIGH; // 空闲状态

    return data;
}

/**
 * 写RC522寄存器
 * @param addr 寄存器地址
 * @param data 要写入的数据
*/
static void writeRawRC(uint8_t addr, uint8_t data)
{
    u8 tempAddr = (addr << 1) & 0x7E; // 最后一位保留不使用，最高位为0表示写入

    SDALOW; // 开始通信
    spiSendByte(tempAddr); // 写入地址
    spiSendByte(data); // 写入数据
    SDAHIGH; // 结束通信
}

/**
 * 读RC522寄存器
 * @param addr 寄存器地址
 * @return 读取到的数据
*/
static uint8_t readRawRC(uint8_t addr)
{
    uint8_t retData = 0x00;
    u8 tempAddr = ((addr << 1) & 0x7E) | 0x80; // 最后一位保留不使用，最高位为1表示读取

    SDALOW; // 开始通信
    spiSendByte(tempAddr); // 写入地址
    retData = spiReadByte(); // 读取数据
    SDAHIGH; // 结束通信

    return retData;
}

/**
 * @Brief   设置寄存器指定位
 * @Para    Reg  寄存器地址
 * @Para    Mask 
*/
static void setBitMask(u8 reg, u8 mask)
{
    u8 temp;

    temp = readRawRC(reg);
    writeRawRC(reg, temp | mask);         // set bit mask
}

/**
 * 清除寄存器指定位
 * @param reg 寄存器地址
 * @param mask 要清除的位
*/
static void clearBitMask(u8 reg, u8 mask)  
{
    u8 temp;

    temp = readRawRC(reg);
    writeRawRC(reg, temp & ( ~ mask));  // clear bit mask
}

static void pcdAntennaOn(void)
{
    u8 uc = 0x00;

    uc = readRawRC(TxControlReg);
    if((uc & 0x03) != 0x03)
        setBitMask(TxControlReg, 0x03);
}

static void pcdAntennaOff(void)
{
    clearBitMask(TxControlReg, 0x03);
}

/*
 * 描述  ：通过RC522和ISO14443卡通讯
 * 输入  ：ucCommand，RC522命令字
 *         pInData，通过RC522发送到卡片的数据
 *         ucInLenByte，发送数据的字节长度
 *         pOutData，接收到的卡片返回数据
 *         pOutLenBit，返回数据的位长度
 * 返回  : 状态值
 *         = MI_OK，成功
 * 调用  ：内部调用
 */
static char pcdComMF522(u8 ucCommand, u8* pInData, u8 ucInLenByte, u8* pOutData, u32* pOutLenBit)
{
    char cStatus = MI_ERR;
    u8 ucIrqEn   = 0x00;
    u8 ucWaitFor = 0x00;
    u8 ucLastBits;
    u8 ucN;
    u32 ul;

    switch(ucCommand)
    {
       case PCD_AUTHENT:		//Mifare认证
          ucIrqEn   = 0x12;		//允许错误中断请求ErrIEn  允许空闲中断IdleIEn
          ucWaitFor = 0x10;		//认证寻卡等待时候 查询空闲中断标志位
          break; 
       case PCD_TRANSCEIVE:		//接收发送 发送接收
          ucIrqEn   = 0x77;		//允许TxIEn RxIEn IdleIEn LoAlertIEn ErrIEn TimerIEn
          ucWaitFor = 0x30;		//寻卡等待时候 查询接收中断标志位与 空闲中断标志位
          break; 
       default:
         break; 
    }
   
    writeRawRC(ComIEnReg, ucIrqEn | 0x80);		//IRqInv置位管脚IRQ与Status1Reg的IRq位的值相反 
    clearBitMask(ComIrqReg, 0x80);			//Set1该位清零时，CommIRqReg的屏蔽位清零
    writeRawRC(CommandReg, PCD_IDLE);		//写空闲命令
    setBitMask(FIFOLevelReg, 0x80);			//置位FlushBuffer清除内部FIFO的读和写指针以及ErrReg的BufferOvfl标志位被清除
    
    for(ul = 0; ul < ucInLenByte; ul ++)
		  writeRawRC(FIFODataReg, pInData[ul]);    		//写数据进FIFOdata
			
    writeRawRC(CommandReg, ucCommand);					//写命令
   
    if(ucCommand == PCD_TRANSCEIVE)
			setBitMask(BitFramingReg,0x80);  				//StartSend置位启动数据发送 该位与收发命令使用时才有效
    
    ul = 1000;//根据时钟频率调整，操作M1卡最大等待时间25ms
		
    do 														//认证 与寻卡等待时间	
    {
         ucN = readRawRC(ComIrqReg);							//查询事件中断
         ul --;
    }while((ul != 0 ) && ( ! ( ucN & 0x01 ) ) && ( ! ( ucN & ucWaitFor)));		//退出条件i=0,定时器中断，与写空闲命令
		
    clearBitMask(BitFramingReg, 0x80);					//清理允许StartSend位
		
    if(ul != 0)
    {
		if(!((readRawRC(ErrorReg) & 0x1B )))			//读错误标志寄存器BufferOfI CollErr ParityErr ProtocolErr
		{
			cStatus = MI_OK;
			
			if(ucN & ucIrqEn & 0x01)					//是否发生定时器中断
			    cStatus = MI_NOTAGERR;
				
			if(ucCommand == PCD_TRANSCEIVE)
			{
				ucN = readRawRC(FIFOLevelReg);			//读FIFO中保存的字节数
				
				ucLastBits = readRawRC(ControlReg) & 0x07;	//最后接收到得字节的有效位数
				
				if(ucLastBits)
					*pOutLenBit = (ucN - 1) * 8 + ucLastBits;   	//N个字节数减去1（最后一个字节）+最后一位的位数 读取到的数据总位数
				else
					*pOutLenBit = ucN * 8;   					//最后接收到的字节整个字节有效
				
				if(ucN == 0)
                    ucN = 1;    
				
				if(ucN > MAXRLEN)
					ucN = MAXRLEN;
				
				for(ul = 0; ul < ucN; ul ++)
				    pOutData[ul] = readRawRC(FIFODataReg);
			}		
        }
		else
			cStatus = MI_ERR;
    }
   
   setBitMask(ControlReg, 0x80);           // stop timer now
   writeRawRC(CommandReg, PCD_IDLE); 
	
   return cStatus;
}

u8 rc522IsConnected(void)
{
    return (readRawRC(VersionReg)==0x92);
}

/*
功    能: 寻卡
参数说明: req_code[IN]:寻卡方式
                0x52   = 寻感应区内所有符合14443A标准的卡
                0x26   = 寻未进入休眠状态的卡
                pTagType[OUT]:卡片类型代码
                0x4400 = Mifare_UltraLight
                0x0400 = Mifare_One(S50)
                0x0200 = Mifare_One(S70)
                0x0800 = Mifare_Pro(X)
                0x4403 = Mifare_DESFire
返 回 值: 成功返回MI_OK
*/
char pcdRequest(u8 req_code,u8 *pTagType)
{
    char status;  
    u32 unLen;
    u8 ucComMF522Buf[MAXRLEN];       // MAXRLEN  18

    clearBitMask(Status2Reg,0x08);  //清RC522寄存器位,/接收数据命令
    writeRawRC(BitFramingReg,0x07); //写RC632寄存器
    setBitMask(TxControlReg,0x03);  //置RC522寄存器位

    ucComMF522Buf[0]=req_code;       //寻卡方式

    status=pcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen); //通过RC522和ISO14443卡通讯

    if((status==MI_OK)&&(unLen==0x10))
    {    
        *pTagType=ucComMF522Buf[0];
        *(pTagType+1)=ucComMF522Buf[1];
    }
    else
    {
        status = MI_ERR;
    }  
    return status;
}

/*
 * 描述  ：防冲撞
 * 输入  ：pSnr，卡片序列号，4字节
 * 返回  : 状态值
 *         = MI_OK，成功
 */
char pcdAnticoll(u8* pSnr)
{
    char cStatus;
    u8 uc, ucSnr_check = 0;
    u8 ucComMF522Buf[MAXRLEN]; 
	u32 ulLen;

    clearBitMask(Status2Reg, 0x08);		//清MFCryptol On位 只有成功执行MFAuthent命令后，该位才能置位
    writeRawRC(BitFramingReg, 0x00);		//清理寄存器 停止收发
    clearBitMask(CollReg, 0x80);			//清ValuesAfterColl所有接收的位在冲突后被清除
   
    ucComMF522Buf[0] = 0x93;	//卡片防冲突命令
    ucComMF522Buf[1] = 0x20;
   
    cStatus = pcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &ulLen);//与卡片通信
	
    if(cStatus == MI_OK)		//通信成功
    {
		for(uc = 0; uc < 4; uc ++)
        {
            *(pSnr + uc) = ucComMF522Buf[uc];			//读出UID
            ucSnr_check ^= ucComMF522Buf[uc];
        }
			
        if(ucSnr_check != ucComMF522Buf[uc])
        		cStatus = MI_ERR;	 
    }
    
    setBitMask(CollReg, 0x80);

    return cStatus;
}

/**
 * 功    能：选定卡片
 * 参数说明：pSnr[IN]:卡片ID，4字节
 * 返    回：成功返回MI_OK
*/
char pcdSelectTag(u8 *pSnr)
{
    char status;
    u8 i;
    u32 unLen;
    u8 ucComMF522Buf[MAXRLEN];
    
    ucComMF522Buf[0]=PICC_ANTICOLL1;
    ucComMF522Buf[1]=0x70;
    ucComMF522Buf[6]=0;
  
    for(i=0;i<4;i++)
    {
        ucComMF522Buf[i+2]=*(pSnr+i);
        ucComMF522Buf[6]^=*(pSnr+i);
    }
    
    calulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]); //用MF522计算CRC16函数，校验数据
    clearBitMask(Status2Reg, 0x08);                  //清RC522寄存器位

    status=pcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);
    if((status==MI_OK)&&(unLen==0x18)) status=MI_OK;
    else status=MI_ERR;
    
    return status;
}

#if 0
/**
 * 功    能：命令卡片进入休眠状态
 * 返    回：成功返回MI_OK
*/
char pcdHalt(void)
{
    u8 status;
    u32 unLen;
    u8 ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0]=PICC_HALT;
    ucComMF522Buf[1]=0;
    calulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status=pcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    return status;
}

/**
 * 功能:选卡读取卡存储器容量
 * 输入参数:serNum 传入卡ID，4字节
 * 返 回 值:成功返回卡容量
*/
u8 pcdGetSize(u8* serNum)
{     
    u8 i;
    u8 status;
    u8 size;
    u32 recvBits;
    u8 buffer[9];
        
    buffer[0]=PICC_ANTICOLL1;    //防撞码1
    buffer[1]=0x70;
    buffer[6]=0x00;

    for(i=0;i<4;i++)
    {
        buffer[i+2]=*(serNum+i);    //buffer[2]-buffer[5]为卡序列号
        buffer[6]^=*(serNum+i);    //卡校验码
    }
  
    calulateCRC(buffer,7,&buffer[7]);  //buffer[7]-buffer[8]为RCR校验码
    clearBitMask(Status2Reg,0x08);

    status=pcdComMF522(PCD_TRANSCEIVE,buffer,9,buffer,&recvBits);
  
    if((status==MI_OK)&&(recvBits==0x18))    
        size=buffer[0];     
    else    
        size=0;
  
    return size;
}
#endif

/**
 * 功    能：验证卡片密码
 * 参数说明：auth_mode[IN]: 密码验证模式
 *                0x60 = 验证A密钥
 *                0x61 = 验证B密钥
 *          addr[IN]：块地址0-63
 *          pKey[IN]：扇区密码，6字节
 *          pSnr[IN]：卡片序列号，4字节
 * 返    回：成功返回MI_OK
*/               
char pcdAuthPasswd(u8 auth_mode,u8 addr,u8* pKey,u8* pSnr)
{
    char status;
    u32 unLen;
    u8 ucComMF522Buf[MAXRLEN];  //MAXRLEN  18(数组的大小)

    //验证模式+块地址+扇区密码+卡序列号
    ucComMF522Buf[0]=auth_mode;
    ucComMF522Buf[1]=addr;
    memcpy(&ucComMF522Buf[2],pKey,6); //拷贝，复制
    memcpy(&ucComMF522Buf[8],pSnr,4);

    status=pcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
    if((status != MI_OK) || (!(readRawRC(Status2Reg)&0x08)))
        status = MI_ERR;

    return status;
}

/**
 * 功    能：读取M1卡一块数据
 * 参数说明：
 *      addr：块地址0-63
 *      p   ：读出的块数据，16字节
 * 返    回：成功返回MI_OK
*/
char pcdReadData(u8 addr,u8* p)
{
    char status;
    u32 unLen;
    u8 i,ucComMF522Buf[MAXRLEN]; //18

    ucComMF522Buf[0]=PICC_READ;
    ucComMF522Buf[1]=addr;
    calulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

    status=pcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
    if((status==MI_OK&&(unLen==0x90)))
    {
        for(i=0;i<16;i++)
            *(p+i)=ucComMF522Buf[i];
    }
    else
    {   
      status=MI_ERR;
    }

    return status;
}

/**
 * 功    能：写数据到M1卡指定块
 * 参数说明：addr：块地址0-63
 *          p   ：向块写入的数据，16字节
 * 返    回：成功返回MI_OK
*/                  
char pcdWriteData(u8 addr,u8* p)
{
    char status;
    u32 unLen;
    u8 i,ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0]=PICC_WRITE;// 0xA0 //写块
    ucComMF522Buf[1]=addr;      //块地址
    calulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status=pcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
    if((status!= MI_OK) || (unLen != 4) || ((ucComMF522Buf[0]&0x0F) != 0x0A))
    {
        status = MI_ERR;
    }
    
    if(status == MI_OK)
    {
        for(i=0;i<16;i++)//向FIFO写16Byte数据 
        {
            ucComMF522Buf[i]=*(p+i);
        }
        calulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

        status = pcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
        if((status != MI_OK)||(unLen != 4) || ((ucComMF522Buf[0]&0x0F)!=0x0A))
        {   
            status = MI_ERR;   
        }
    }

    return status;
}

/**
 * 功    能：用MF522计算CRC16函数
 * 参    数：
 *          *pIn ：要读数CRC的数据
 *          len：数据长度
 *          *pOut：计算的CRC结果
*/
static void calulateCRC(u8* pIn, u8 len,u8* pOut)
{
    u8 i,n;

    clearBitMask(DivIrqReg,0x04);  //CRCIrq = 0  
    writeRawRC(CommandReg,PCD_IDLE);
    setBitMask(FIFOLevelReg,0x80); //清FIFO指针
    
    //向FIFO中写入数据  
    for(i=0;i<len;i++)
    {
        writeRawRC(FIFODataReg,*(pIn + i));  //开始RCR计算
    }
    
    writeRawRC(CommandReg,PCD_CALCCRC);   //等待CRC计算完成 
    
    i=0xFF;
    do 
    {
        n=readRawRC(DivIrqReg);
        i--;
    }
    while((i!=0)&&!(n&0x04)); //CRCIrq = 1
    
    //读取CRC计算结果 
    pOut[0]=readRawRC(CRCResultRegL);
    pOut[1]=readRawRC(CRCResultRegM);
}

