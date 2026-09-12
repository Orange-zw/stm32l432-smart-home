#include "esp8266.h"
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os 使用	  
#endif

//WIFI STA模式,设置要去连接的路由器无线参数,请根据你自己的路由器设置,自行修改.
const u8* wifista_ssid="CKJ";			//路由器SSID号
const u8* wifista_password="15219712622"; 	//连接密码

const u8* server_ip="192.168.1.1";			//服务器IP
const u8* server_port="9999"; 	//服务器端口

//ESP8266状态，位7：1为已连接WIFI
//位4-5：00为ID号为0的远程命令需要解析，01、10、11以此类推
//位2-0：000为复位状态；001为接收到'+'号，当下一接收字符为'I'时开始存放命令；010为接收到"+I"，准备接收对方ID号
//011为接收到ID号，准备接收命令；100为接收命令中，准备完成接收；101为当前命令接收完毕；
u8 ESP8266_STA;
//存放远程命令的字符串
u8 ESP8266_Cmd[50];


void esp8266_init()
{
	while(esp8266_send_cmd("ATE0","OK",20));  //关回显
	esp8266_send_cmd("AT+CWMODE=1","OK",20);  //STA模式
	esp8266_send_cmd("AT+CIPMUX=1","OK",20);  //多连接模式
	sprintf((char*)ESP8266_TX_BUF,"AT+CWJAP=\"%s\",\"%s\"\r\n",wifista_ssid,wifista_password);
	esp8266_send_data(ESP8266_TX_BUF,"OK",8000);  //连接wifi
	while(Check_Connections() != 2 && Check_Connections() != 4);  //等待WIFI连接完成
	sprintf((char*)ESP8266_TX_BUF,"AT+CIPSTART=0,\"TCP\",\"%s\",%s\r\n",server_ip,server_port);
	esp8266_send_data(ESP8266_TX_BUF,"OK",50);  //连接服务器
}


//ATK-ESP8266发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//    其他,期待应答结果的位置(str的位置)
u8* esp8266_check_cmd(u8 *str)
{
	char *strx=0;
	if(ESP8266_RX_STA&0X8000)		//接收到一次数据了
	{ 
		ESP8266_RX_BUF[ESP8266_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)ESP8266_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//向ATK-ESP8266发送命令
//cmd:发送的命令字符串
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 esp8266_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	ESP8266_RX_STA=0;
	esp8266_printf("%s\r\n",cmd);	//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			delay_ms(10);
			if(ESP8266_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(esp8266_check_cmd(ack))
				{
//					printf("ack:%s\r\n",(u8*)ack);
					break;//得到有效数据 
				}
					ESP8266_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 
//向ATK-ESP8266发送指定数据
//data:发送的数据(不需要添加回车了)
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)luojian
u8 esp8266_send_data(u8 *data,u8 *ack,u16 waittime)
{
	u8 res=0; 
	ESP8266_RX_STA=0;
	esp8266_printf("%s",data);	//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			delay_ms(10);
			if(ESP8266_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(esp8266_check_cmd(ack))break;//得到有效数据 
				ESP8266_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
}

//等待ESP8266应答
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 Wait_Reply(u8 *ack,u16 waittime)
{
	ESP8266_RX_STA=0;
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			delay_ms(10);
			if(ESP8266_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(esp8266_check_cmd(ack))break;//得到有效数据 
				ESP8266_RX_STA=0;
			} 
		}
		if(waittime==0) return 1; 
	}
	return 0;
}

//自动向当前ID发送数据
//发送成功返回1，失败返回0
u8 Send_NetworkData(u8 ID,u8* data)
{
	u8 data_buf[22];
	sprintf((char*)data_buf,"AT+CIPSENDEX=%1d,%d\r\n",ID,strlen((const char*)data));
	WIFI_SendData(data_buf);
//	printf("AT+CIPSENDEX=%1d,%d",ID,strlen((const char*)data));
	if(!Wait_Reply(">",10) && !esp8266_send_data(data,"SEND OK",30))
	{
		return 1;
	}
	return 0;
}

//检查esp8266的连接情况并返回连接状态
//返回值：0:状态获取失败
//2 已连接AP，获得IP地址
//3：已建立TCP或UDP连接
//4：断开网络连接
//5：未连接AP
u8 Check_Connections()
{
	if(!esp8266_send_cmd("AT+CIPSTATUS","STATUS:",10))
		return ESP8266_RX_BUF[7]-0x30;
	return 0;
}

//ATK-ESP8266退出透传模式
//返回值:0,退出成功;
//       1,退出失败
u8 esp8266_quit_trans(void)
{
	while((USART1->ISR&0X40)==0);	//等待发送空
	USART1->TDR='+';      
	delay_ms(15);					//大于串口组帧时间(10ms)
	while((USART1->ISR&0X40)==0);	//等待发送空
	USART1->TDR='+';      
	delay_ms(15);					//大于串口组帧时间(10ms)
	while((USART1->ISR&0X40)==0);	//等待发送空
	USART1->TDR='+';      
	delay_ms(500);					//等待500ms
	return esp8266_send_cmd("AT","OK",20);//退出透传判断.
}

//获取ATK-ESP8266模块的AP+STA连接状态
//返回值:0，未连接;1,连接成功
u8 esp8266_apsta_check(void)
{
	if(esp8266_quit_trans())return 0;			//退出透传 
	esp8266_send_cmd("AT+CIPSTATUS",":",50);	//发送AT+CIPSTATUS指令,查询连接状态
	if(esp8266_check_cmd("+CIPSTATUS:0")&&
		 esp8266_check_cmd("+CIPSTATUS:1")&&
		 esp8266_check_cmd("+CIPSTATUS:2")&&
		 esp8266_check_cmd("+CIPSTATUS:4"))
		return 0;
	else return 1;
}
//获取ATK-ESP8266模块的连接状态
//返回值:0,未连接;1,连接成功.
u8 esp8266_consta_check(void)
{
	u8 *p;
	u8 res;
	if(esp8266_quit_trans())return 0;			//退出透传 
	esp8266_send_cmd("AT+CIPSTATUS",":",50);	//发送AT+CIPSTATUS指令,查询连接状态
	p=esp8266_check_cmd("+CIPSTATUS:"); 
	res=*p;									//得到连接状态	
	return res;
}

//获取Client ip地址
//ipbuf:ip地址输出缓存区
void esp8266_get_wanip(u8* ipbuf)
{
	u8 *p,*p1;
		if(esp8266_send_cmd("AT+CIFSR","OK",50))//获取WAN IP地址失败
		{
			ipbuf[0]=0;
			return;
		}		
		p=esp8266_check_cmd("\"");
		p1=(u8*)strstr((const char*)(p+1),"\"");
		*p1=0;
		sprintf((char*)ipbuf,"%s",p+1);	
}

//命令扫描
void Cmd_Scan()
{
	u8 num=0,i=1,j=0;
	if((ESP8266_STA&7)==5)
	{
		if(ESP8266_Cmd[0]=='{')
		{
			for(;i<50;i++)
			{
				if(ESP8266_Cmd[i]==',') num++;
				if(num==7)
				{
					j=++i;
					while(ESP8266_Cmd[j++]!=',' && j-i<4);
					switch(j-i-1)
					{
						case 1:Manual_STA=ESP8266_Cmd[i]-0x30;
										break;
						case 2:Manual_STA=(ESP8266_Cmd[i]-0x30)*10 + (ESP8266_Cmd[i+1]-0x30);
										break;
						case 3:Manual_STA=(ESP8266_Cmd[i]-0x30)*100 + (ESP8266_Cmd[i+1]-0x30)*10 + (ESP8266_Cmd[i+2]-0x30);
										break;
					}
//					printf("len:%d  %s\r\n",j-i,&ESP8266_Cmd[i]);
					i=j;
					while(ESP8266_Cmd[j++]!='}' && j-i<4);
					switch(j-i-1)
					{
						case 1:AUTO_STA=ESP8266_Cmd[i]-0x30;
										break;
						case 2:AUTO_STA=(ESP8266_Cmd[i]-0x30)*10 + (ESP8266_Cmd[i+1]-0x30);
										break;
						case 3:AUTO_STA=(ESP8266_Cmd[i]-0x30)*100 + (ESP8266_Cmd[i+1]-0x30)*10 + (ESP8266_Cmd[i+2]-0x30);
										break;
					}
//					printf("len:%d  %s\r\n",j-i,&ESP8266_Cmd[i]);
//					printf("%d  %d\r\n",Manual_STA,AUTO_STA);
					break;
				}
			}
		}
	}
}





