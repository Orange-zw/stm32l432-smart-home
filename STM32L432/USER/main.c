#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "usart2.h"
#include "led.h"
#include "includes.h"
#include "esp8266.h"
#include "adc.h"
#include "DS18B20.h"

//任务优先级
#define START_TASK_PRIO		3
//任务堆栈大小	
#define START_STK_SIZE 		512
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);

//任务优先级
#define LED0_TASK_PRIO		4
//任务堆栈大小	
#define LED0_STK_SIZE 		256
//任务控制块
OS_TCB Led0TaskTCB;
//任务堆栈	
CPU_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *p_arg);

//任务优先级
#define LED1_TASK_PRIO		5
//任务堆栈大小	
#define LED1_STK_SIZE       256
//任务控制块
OS_TCB Led1TaskTCB;
//任务堆栈	
CPU_STK LED1_TASK_STK[LED1_STK_SIZE];
//任务函数
void led1_task(void *p_arg);

//任务优先级
#define FLOAT_TASK_PRIO		6
//任务堆栈大小
#define FLOAT_STK_SIZE		256
//任务控制块
OS_TCB	FloatTaskTCB;
//任务堆栈
__align(8) CPU_STK	FLOAT_TASK_STK[FLOAT_STK_SIZE];
//任务函数
void float_task(void *p_arg);

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	HAL_Init();                     //初始化HAL库
	Stm32_Clock_Init(20,1,RCC_PLLP_DIV7,RCC_PLLQ_DIV2,RCC_PLLR_DIV2);
	delay_init(80);                //初始化延时函数
	uart_init(115200);
	uart2_init(2400);              //初始化USART
	LED_Init();                     //初始化LED 
	MY_ADC_Init();
//		TIM7->CR1|=1<<0;
	
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_SAVE_FP, //任务选项,为了保险起见，所有任务都保存浮点寄存器的值
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);      //开启UCOSIII
	while(1)
	{
	} 
}

//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,设置默认的时间片长度s
	OSSchedRoundRobinCfg(DEF_ENABLED,10,&err);  
#endif		
	
	OS_CRITICAL_ENTER();	//进入临界区

	OSTaskCreate((OS_TCB 	* )&Led0TaskTCB,		
				 (CPU_CHAR	* )"led0 task", 		
                 (OS_TASK_PTR )led0_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED0_TASK_PRIO,     
                 (CPU_STK   * )&LED0_TASK_STK[0],	
                 (CPU_STK_SIZE)LED0_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED0_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_SAVE_FP,
                 (OS_ERR 	* )&err);				
				 
	OSTaskCreate((OS_TCB 	* )&Led1TaskTCB,		
				 (CPU_CHAR	* )"led1 task", 		
                 (OS_TASK_PTR )led1_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )LED1_TASK_PRIO,     	
                 (CPU_STK   * )&LED1_TASK_STK[0],	
                 (CPU_STK_SIZE)LED1_STK_SIZE/10,	
                 (CPU_STK_SIZE)LED1_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_SAVE_FP, 
                 (OS_ERR 	* )&err);
				 
	OSTaskCreate((OS_TCB 	* )&FloatTaskTCB,		
				 (CPU_CHAR	* )"float test task", 		
                 (OS_TASK_PTR )float_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )FLOAT_TASK_PRIO,     	
                 (CPU_STK   * )&FLOAT_TASK_STK[0],	
                 (CPU_STK_SIZE)FLOAT_STK_SIZE/10,	
                 (CPU_STK_SIZE)FLOAT_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,					
                 (void   	* )0,				
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_SAVE_FP, 
                 (OS_ERR 	* )&err);
	OS_CRITICAL_EXIT();	//退出临界区			 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
}

//电器控制任务函数
void led0_task(void *p_arg)
{
	OS_ERR err;
//	p_arg = p_arg;
	while(1)
	{
		GPIOB->ODR ^= 1<<3;
		OSTimeDlyHMSM(0,0,0,300,OS_OPT_TIME_HMSM_STRICT,&err); 
		
		Control();
	}
}

//数据采集发送任务函数
void led1_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	
//	p_arg = p_arg;
	
	OS_CRITICAL_ENTER();	//进入临界区
	DS18B20_Init();
	OS_CRITICAL_EXIT();		//退出临界区
	
	esp8266_init();  //esp8266模块初始化
	while(1)
	{
		//采集MQ2和MQ135的数据等级
		MQ2_Air_Quality=MQ2_Collect(20);
		MQ135_Air_Quality=MQ135_Collect(20);
		
		//DS18B20采集时时序不能断，所以要进入临界区
		OS_CRITICAL_ENTER();	//进入临界区
		DS18B20_Temperature=DS18B20_Get_Temp();
		OS_CRITICAL_EXIT();		//退出临界
		
		//判断PM2.5是否采集完毕
		if(GP2Y1050_Times>=GP2Y1050_COLLECT_TIMES)
		{
			if(GP2Y1050_Quality<100) PM2_5_Quality='A';
			else if(GP2Y1050_Quality<150) PM2_5_Quality='B';
			else PM2_5_Quality='C';		
//			printf("GP2Y1050_Quality:%f\r\n",GP2Y1050_Quality);
			GP2Y1050_Times=0;
			__HAL_UART_ENABLE_IT(&UART2_Handler,UART_IT_RXNE);
		}
		
		//组合各参数并发送至远程服务器，之所以搞这么长是因为手机APP程序不是本方案定制的，有些留0以兼容APP
		sprintf((char*)ESP8266_TX_BUF,"{0,%c,%c,0,0,%2.1f°C,%d,%d,0,%1d,%1d,%1d,0,0,0,0,%c}",
			MQ2_Air_Quality?MQ2_Air_Quality:'0',MQ135_Air_Quality?MQ135_Air_Quality:'0',DS18B20_Temperature,Manual_STA,AUTO_STA,
		(GPIOA->ODR&(1<<8))?0:1,(GPIOA->ODR&(1<<11))?1:0,(GPIOB->ODR&(1<<4))?128:0,PM2_5_Quality?PM2_5_Quality:'0');
		Send_NetworkData(0,ESP8266_TX_BUF);//数据发送至服务器
		
		OSTimeDlyHMSM(0,0,3,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时2s
	}
}

//远程命令解析任务
void float_task(void *p_arg)
{
	OS_ERR err;
//	CPU_SR_ALLOC();
//	static float float_num=0.00;
	while(1)
	{
		
		
		//命令扫描
		if((ESP8266_STA&7)==5)
		{
//			printf("ESP8266_CMD:%s\r\n",ESP8266_Cmd);
			Cmd_Scan();
			ESP8266_STA&=(1<<7);
		}
		OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_HMSM_STRICT,&err); //延时20ms
	}
}
