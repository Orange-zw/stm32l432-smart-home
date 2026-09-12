#include "usart.h"

#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os 使用	  
#endif

//串口发送缓存区 	
__align(8) u8 ESP8266_TX_BUF[ESP8266_MAX_SEND_LEN]; 	//发送缓冲,最大ESP8266_MAX_SEND_LEN字节  	  
//串口接收缓存区 	
u8 ESP8266_RX_BUF[ESP8266_MAX_RECV_LEN]; 				//接收缓冲,最大ESP8266_MAX_RECV_LEN个字节.


//通过判断接收连续2个字符之间的时间差不大于100ms来决定是不是一次连续的数据.
//如果2个字符接收间隔超过100ms,则认为不是1次连续数据.也就是超过100ms没有接收到
//任何数据,则表示此次接收完毕.
//接收到的数据状态
//[15]:0,没有接收到数据;1,接收到了一批数据.
//[14:0]:接收到的数据长度
u16 ESP8266_RX_STA=0; 
UART_HandleTypeDef UART1_Handler; //UART句柄
//u8 USART2_RX_BUF[1];

u8 ESP8266_REC_BUF;

//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound)
{	
	UART1_Handler.Instance = USART1;
  UART1_Handler.Init.BaudRate = bound;
  UART1_Handler.Init.WordLength = UART_WORDLENGTH_8B;
  UART1_Handler.Init.StopBits = UART_STOPBITS_1;
  UART1_Handler.Init.Parity = UART_PARITY_NONE;
  UART1_Handler.Init.Mode = UART_MODE_TX_RX;
  UART1_Handler.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  UART1_Handler.Init.OverSampling = UART_OVERSAMPLING_16;
  UART1_Handler.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  UART1_Handler.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT|UART_ADVFEATURE_DMADISABLEONERROR_INIT;
  UART1_Handler.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  UART1_Handler.AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
	HAL_UART_Init(&UART1_Handler);					    //HAL_UART_Init()会使能UART1
//	HAL_UART_Receive_IT(&UART1_Handler,ESP8266_RX_BUF, 10);//该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量
	
		TIM7_Int_Init(600-1,8000-1);		//60ms中断
		ESP8266_RX_STA=0;		//清零
		TIM7->CR1&=~(1<<0);        //关闭定时器7
}

//串口1,printf 函数
//确保一次发送数据不超过ESP8266_MAX_SEND_LEN字节
void esp8266_printf(char* fmt,...)  
{  
	u16 i,j; 
	va_list ap; 
	va_start(ap,fmt);
	vsprintf((char*)ESP8266_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)ESP8266_TX_BUF);		//此次发送数据的长度
	for(j=0;j<i;j++)							//循环发送数据
	{
		while((USART1->ISR&0X40)==0);			//循环发送,直到发送完毕   
		USART1->TDR=ESP8266_TX_BUF[j];  
	} 
}

//串口1发送字符串
void WIFI_SendData(u8* data)
{
	u8 i;
	u16 num;
	num = strlen((const char*)data);
	for(i=0;i<num;i++)
	{
		while((USART1->ISR&0X40)==0);//等待发送结束
		USART1->TDR=data[i];
	}
}

//串口1中断函数
void USART1_IRQHandler(void)
{
	u8 res;
	static u8 i=0;
#if SYSTEM_SUPPORT_OS	 	//使用OS
	OSIntEnter();    
#endif	
	if(__HAL_UART_GET_FLAG(&UART1_Handler,UART_FLAG_RXNE)!=RESET)//接收到数据
	{
		__HAL_UART_CLEAR_FLAG(&UART1_Handler,UART_FLAG_RXNE);
//		HAL_UART_Receive(&UART1_Handler,&res,1,1000);
		res=USART1->RDR; 
//		printf("%c %d\r\n",res,res);	
		if((ESP8266_STA&7) < 5)
		{
			switch (ESP8266_STA&7)
			{
				case 0:if(res=='+')
								{
									ESP8266_STA|=1;
									i=0;
								}
								break;
				case 1:if(res=='I')
								{
									ESP8266_STA&=~7;
									ESP8266_STA|=2;
									memset(ESP8266_Cmd,0,50);
									ESP8266_RX_STA=0;
//								USART1->TDR='2';
								}
								else ESP8266_STA&=~7;
								break;
				case 2:if(res>='0' && res<='3')
								{
									ESP8266_STA&=~7;
									ESP8266_STA|=3;
									ESP8266_STA|=(res-0x30)<<4;
//								USART1->TDR='3';
								}
								else if(res=='4')
								{
									ESP8266_STA&=~7;
									ESP8266_STA|=11;
								}
								break;
				case 3:if(res==':')
								{
									ESP8266_STA&=~7;
									ESP8266_STA|=4;
								USART1->TDR='4';
								}
								break;
				case 4:if(i<49)
								{
									ESP8266_Cmd[i++]=res;
								}
								else
								{
									ESP8266_STA&=~7;
									ESP8266_STA|=5;
								}
								break;
			}
		}
		
		if((ESP8266_RX_STA&(1<<15))==0)//接收完的一批数据,还没有被处理,则不再接收其他数据
		{ 
			if(ESP8266_RX_STA<ESP8266_MAX_RECV_LEN)	//还可以接收数据
			{
//				__HAL_TIM_SetCounter(&TIM7_Handler,0);	
				TIM7->CNT=0;         				//计数器清空	
				if(ESP8266_RX_STA==0) 				//使能定时器7的中断 
				{
//					__HAL_RCC_TIM7_CLK_ENABLE();            //使能TIM7时钟
					TIM7->CR1|=1<<0;     			//使能定时器7
				}
				ESP8266_RX_BUF[ESP8266_RX_STA++]=res;	//记录接收到的值	 
			}else 
			{
				ESP8266_RX_STA|=1<<15;				//强制标记接收完成
			} 
		}
	}	
#if SYSTEM_SUPPORT_OS	 	//使用OS
	OSIntExit();  											 
#endif	
}   




