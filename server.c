#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include <unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <pthread.h>
#define DEFAULT_PORT 9999  //父进程监听端口  硬件端口
#define DEFAULT_PORT_1 8001  //子进程监听端口  手机APP端口
#define MAXLINE 4096  
#define READ    0  
#define WRITE   1
#define CHILD   0  
#define FATHER  1

//该程序在ubuntu下使用命令gcc -o server server.c -lpthread进行编译，编译后直接运行即可，如端口号冲突可在宏定义中修改

void  receive(char *ptr);
void  read_pipe(char *ptr);
void  sleep_clean(char *ptr);

int retval;
unsigned int count;
char    buff[500]; //TCP接收缓存
char    buff_pipe[500]; //管道接收缓存
int    socket_fd, connect_fd;
int fwcr[2],frcw[2];
unsigned char TCP_state,pthread_state,process_state;
pthread_t thread,thread_pipe,clean_thread;
int main(int argc, char** argv)
{

        /*初始化管道*/
        pipe(fwcr);
        pipe(frcw);
        int pid_x = fork();
        if(pid_x < 0)
        {
                printf("Create process error!\n");
                exit(0);
        }
        if(pid_x==0)  {
        struct sockaddr_in     servaddr;
        close(fwcr[WRITE]);
        close(frcw[READ]);
        process_state = CHILD;  //标记子进程
 
        //初始化  
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。  
        servaddr.sin_port = htons(DEFAULT_PORT_1);//设置的端口为DEFAULT_PORT  



        pthread_create(&thread_pipe,NULL, (void *)(&read_pipe), (void *)0);
        pthread_create(&clean_thread,NULL, (void *)(&sleep_clean), (void *)0);

	//初始化Socket  
	if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
		printf("create socket_1 error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
	}
	//将本地地址绑定到所创建的套接字上  
	if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		printf("bind socket_1 error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
	}

        while(1){
                switch(TCP_state)
                {
                case 0: 
			//开始监听是否有客户端连接  
			if( listen(socket_fd, 10) == -1){
				printf("listen socket_1 error: %s(errno: %d)\n",strerror(errno),errno);
				exit(0);
			}
			printf("======waiting for client_1's request======\n");
			//阻塞直到有客户端连接，不然多浪费CPU资源。  
                        if( (connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){
                                printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
                                continue;
                        }
                        else{
                                TCP_state = 1;
                                printf("客户端%d已连接\n",process_state);
                                send(connect_fd, "Hello,you are connected!\n", 26,0);
                        }
                        break;
                case 1://printf("链接中\n");
                        if(buff[0]){
                                write(frcw[1],buff,strlen(buff));
                                //send(connect_fd, buff, 26,0);//向客户端发送回应数据  
                                memset(&buff, 0, 500);
                                printf("%d管道写入成功\n",process_state);
                        }
                        if(!pthread_state){
                                pthread_create(&thread,NULL, (void *)(&receive), (void *)0);
                                pthread_state = 1;
                        }
                        if(buff_pipe[0]){
                                send(connect_fd, buff_pipe,strlen(buff_pipe),0);
                                printf("管道%d数据发送至客户端\n",process_state);
                                memset(&buff_pipe, 0, 500);
                        }
                        break;
                }
        }
        close(socket_fd);
        }
        else {
        struct sockaddr_in     servaddr;
        process_state = FATHER;    //标记父进程
        close(frcw[WRITE]);
        close(fwcr[READ]);
   
        //初始化  
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。  
        servaddr.sin_port = htons(DEFAULT_PORT);//设置的端口为DEFAULT_PORT  

 

	
        pthread_create(&thread_pipe,NULL, (void *)(&read_pipe), (void *)0);
        pthread_create(&clean_thread,NULL, (void *)(&sleep_clean), (void *)0);

	//初始化Socket  
	if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
		printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
	}       
	//将本地地址绑定到所创建的套接字上  
	if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
	}

        while(1){
                switch(TCP_state)
                {
                case 0: 
			//开始监听是否有客户端连接  
			if( listen(socket_fd, 10) == -1){
				printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
				exit(0);
			}
			printf("======waiting for client's request======\n");
			
			
			//阻塞直到有客户端连接，不然多浪费CPU资源。  
                        if( (connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){
                                printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
                                continue;
                        }
                        else{
                                TCP_state = 1;
                                printf("客户端%d已连接\n",process_state);
                                send(connect_fd, "Hello,you are connected!\n", 26,0);
                        }
                        break;
                case 1://printf("链接中\n");
                        if(buff[0]){
                                write(fwcr[1],buff,strlen(buff));
                               //send(connect_fd, buff, 26,0);//向客户端发送回应数据  
                                memset(&buff, 0, 500);
                                printf("%d管道写入成功\n",process_state);
                        }
                        if(buff_pipe[0]){
                                send(connect_fd, buff_pipe,strlen(buff_pipe),0);
                                printf("管道%d数据发送至客户端\n",process_state);
                                memset(&buff_pipe, 0, 500);
                        }
                        if(!pthread_state){
                                pthread_create(&thread,NULL, (void *)(&receive), (void *)0);
                                pthread_state = 1;
                        }
                        break;
                }
        }
        close(socket_fd);
        }
}

void  receive(char *ptr)
{
        int n;
        while(1){
                n = recv(connect_fd, buff, MAXLINE, 0); //接受客户端传过来的数据
                if(n<=0)
                {
                        printf("客户端%d断开\n",process_state);
                        TCP_state = 0;
                        buff[0] = 0;
                        close(connect_fd);
                        pthread_state = 0;
                        pthread_exit(&retval);
                }
		if(process_state) count=0;    //父进程心跳计数复原
                buff[n] = '\0';
                printf("接收完毕:%s",buff);
        }

}

void  read_pipe(char *ptr)
{

        int n;
        printf("管道线程%d成功\n",process_state);
        while(1){
                if(process_state == CHILD) n = read(fwcr[READ],buff_pipe,500);
                else n = read(frcw[READ],buff_pipe,500);
                printf("管道%d读取完毕\n",process_state);
                if(n<=0)
                {
                        printf("读取失败\n");
			TCP_state = 0;
			close(connect_fd);
			if(process_state == FATHER) write(fwcr[1],"+-+-",4);
			else write(frcw[1],"+-+-",4);
                }
								if(!process_state) count=0;    //子进程心跳计数复原
		if(buff_pipe[0]=='+' && buff_pipe[1]=='-' && buff_pipe[2]=='+' && buff_pipe[3]=='-'){
		TCP_state = 0;
		close(connect_fd);
		}
        }

}

void  sleep_clean(char *ptr)
{
	
        while(1){
                sleep(3);
		if(TCP_state){
			 count++;
			//心跳机制  7*3=21s
			//当count=7时断开与客户端的连接
			if(count==7){
				count = 0;
				TCP_state = 0;
				buff[0] = 0;
				close(connect_fd);
				pthread_cancel(thread);
				pthread_state = 0;
				printf("客户端%d超时\n",process_state);
			}	
		}                
		memset(&buff, 0, 500);
                memset(&buff_pipe, 0, 500);
        }
}



