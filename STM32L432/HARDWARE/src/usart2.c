#include "usart2.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用os,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os 使用	  
#endif
  
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
//#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)	
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART2->ISR&0X40)==0);//循环发送,直到发送完毕   
	USART2->TDR = (u8) ch;      
	return ch;
}
#endif 

#if EN_USART2_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
#endif	
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
//u16 USART_RX_STA=0;       //接收状态标记	


//u8 aRxBuffer[RXBUFFERSIZE];//HAL库使用的串口接收缓冲
UART_HandleTypeDef UART2_Handler; //UART句柄

//初始化IO 串口2 
//bound:波特率
void uart2_init(u32 bound)
{	
	//UART 初始化设置
	UART2_Handler.Instance=USART2;					    //USART1
	UART2_Handler.Init.BaudRate = bound;
  UART2_Handler.Init.WordLength = UART_WORDLENGTH_8B;
  UART2_Handler.Init.StopBits = UART_STOPBITS_1;
  UART2_Handler.Init.Parity = UART_PARITY_NONE;
  UART2_Handler.Init.Mode = UART_MODE_TX_RX;
  UART2_Handler.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  UART2_Handler.Init.OverSampling = UART_OVERSAMPLING_16;
  UART2_Handler.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  UART2_Handler.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT|UART_ADVFEATURE_DMADISABLEONERROR_INIT;
  UART2_Handler.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  UART2_Handler.AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
	HAL_UART_Init(&UART2_Handler);					    //HAL_UART_Init()会使能UART1
	
//	HAL_UART_Receive_IT(&UART2_Handler, (u8 *)aRxBuffer, RXBUFFERSIZE);//该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量
  
}

//UART底层初始化，时钟使能，引脚配置，中断配置
//此函数会被HAL_UART_Init()调用
//huart:串口句柄
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO端口设置
	if(huart==(&UART1_Handler))
	{
    //GPIO端口设置
		GPIO_InitTypeDef GPIO_Initure;
	
		__HAL_RCC_GPIOA_CLK_ENABLE();			//使能GPIOA时钟
		__HAL_RCC_USART1_CLK_ENABLE();			//使能USART1时钟
	
		GPIO_Initure.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_Initure.Mode = GPIO_MODE_AF_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Initure.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_Initure);
	
//    __HAL_UART_DISABLE_IT(huart,UART_IT_TC);
#if ESP8266_RX_EN
		__HAL_UART_ENABLE_IT(huart,UART_IT_RXNE);		//开启接收中断
    HAL_NVIC_SetPriority(USART1_IRQn, 2, 2);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
#endif	
	}
	if(huart==(&UART2_Handler))
	{
		  //GPIO端口设置
		GPIO_InitTypeDef GPIO_Initure;
	
		__HAL_RCC_GPIOA_CLK_ENABLE();			//使能GPIOA时钟
		__HAL_RCC_USART2_CLK_ENABLE();			//使能USART2时钟
	
		GPIO_Initure.Pin = GPIO_PIN_2;
    GPIO_Initure.Mode = GPIO_MODE_AF_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Initure.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_Initure);
		
//		GPIO_Initure.Pin = GPIO_PIN_15;
//		GPIO_Initure.Alternate = GPIO_AF3_USART2;
		GPIO_Initure.Pin = GPIO_PIN_3;
		GPIO_Initure.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_Initure);
	
//		__HAL_UART_DISABLE_IT(huart,UART_IT_TC);
#if EN_USART2_RX 
		__HAL_UART_ENABLE_IT(huart,UART_IT_RXNE);		//开启接收中断
		HAL_NVIC_SetPriority(USART2_IRQn,2,3);			//抢占优先级2，子优先级3
		HAL_NVIC_EnableIRQ(USART2_IRQn);				//使能USART2中断
#endif
	}
}

//GP2Y1050转换次数
u8 GP2Y1050_Times;
//PM2.5实时浓度
float GP2Y1050_Quality;
//GP2Y1050接收缓存
u8 GP2Y1050_buf[5];
//GP2Y1050接收状态
//bit7 接收到数据头0XAA，说明可以开始接收数据
//bit6~0 接收数据数目
u8 GP2Y1050_STA=0;

//串口2中断服务程序
void USART2_IRQHandler(void)                	
{ 
	u8 Res;
#if SYSTEM_SUPPORT_OS	 	//使用OS
	OSIntEnter();    
#endif
	#if EN_USART2_RX 
	if((__HAL_UART_GET_FLAG(&UART2_Handler,UART_FLAG_RXNE)!=RESET))  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
//    HAL_UART_Receive(&UART2_Handler,&Res,1,1000); 
		Res=USART2->RDR;
		
		//GP2Y1050接收程序
		if(GP2Y1050_STA&(1<<7) && GP2Y1050_Times<GP2Y1050_COLLECT_TIMES)
		{
			if((GP2Y1050_STA&0x7f)<5)
			{
				GP2Y1050_buf[GP2Y1050_STA&0x7f]=Res;
				GP2Y1050_STA++;
			}
			else if(Res==0xff)
			{
				if(GP2Y1050_buf[4]==GP2Y1050_buf[0]+GP2Y1050_buf[1]+GP2Y1050_buf[2]+GP2Y1050_buf[3])  //数据校验
				{
					GP2Y1050_Times++;
					GP2Y1050_Quality += (GP2Y1050_buf[0]<<8) + GP2Y1050_buf[1];
					if(GP2Y1050_Times==GP2Y1050_COLLECT_TIMES) //达到采集次数后关闭中断
					{
						__HAL_UART_DISABLE_IT(&UART2_Handler,UART_IT_RXNE);		//关闭接收中断
						GP2Y1050_Quality = GP2Y1050_Quality/GP2Y1050_COLLECT_TIMES/1024.0*5;
						GP2Y1050_Quality = GP2Y1050_Quality*600;
					}
				}
				GP2Y1050_STA=0;
			}
			else
			{
				GP2Y1050_STA=0;
			}
		}
		else if(Res==0xaa && GP2Y1050_Times<GP2Y1050_COLLECT_TIMES)
		{
			GP2Y1050_STA|=1<<7;
		}
	 		 
	}
	#endif	
#if SYSTEM_SUPPORT_OS	 	//使用OS
	OSIntExit();  											 
#endif
} 

 




