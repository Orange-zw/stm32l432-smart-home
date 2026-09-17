#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "as608.h"
#include "ring_buf.h"

volatile ringbuf_t *rx_ringbuf;
HANDLE hComm;
HANDLE hReadThread;                  // 读取线程的句柄
volatile BOOL bThreadRunning = TRUE; // 控制线程运行的标志

typedef uint16_t u16;
typedef uint8_t u8;
typedef uint32_t u32;

DCB dcbSerialParams = {0};
COMMTIMEOUTS timeouts = {0};

// 配置串口参数
char portName[] = "COM6";               // 修改为你的实际COM口
DWORD baudRate = CBR_57600;             // 波特率
char sendData[] = "Hello from PC!\r\n"; // 要发送的数据
char receiveBuffer[256] = {0};          // 接收缓冲区
DWORD bytesWritten, bytesRead;

DWORD WINAPI ReadThreadProc(LPVOID lpParam)
{
    HANDLE hCommPort = (HANDLE)lpParam;
    DWORD dwBytesRead = 0;
    u8 temp_buffer[256];

    while (bThreadRunning)
    {
        // 从串口读取数据
        if (ReadFile(hCommPort, temp_buffer, sizeof(temp_buffer), &dwBytesRead, NULL))
        {
            if (dwBytesRead > 0)
            {
                // 将读取的数据写入环形缓冲区
                ringbuf_write(rx_ringbuf, temp_buffer, dwBytesRead);
                printf("接收到 %lu 字节数据\n", dwBytesRead);
                // printf("buf: %p, size: %u, tail: %u, head: %u, full: %d\n",
                //        rx_ringbuf->buffer, rx_ringbuf->size, rx_ringbuf->tail,
                //        rx_ringbuf->head, rx_ringbuf->full);
                for (int i = 0; i < dwBytesRead; i++)
                {
                    printf("%02X ", temp_buffer[i]);
                }
                printf("\n");
            }
        }

        // 短暂休眠，避免CPU占用过高
        Sleep(5);
    }
    return 0;
}

// 串口发送一个字节
void MYUSART_SendData(u8 data)
{
    DWORD bytesSent;
    // printf("%02X ", data); // 打印发送的字节
    if (!WriteFile(hComm, &data, 1, &bytesSent, NULL) || bytesSent != 1)
    {
        printf("发送字节失败，错误代码: %d\n", GetLastError());
    }
}

void MYUSART_ReceiveData(u8 *data, u16 len)
{
    printf("等待接收 %u 字节数据...\n", len);
    while (ringbuf_get_len(rx_ringbuf) < len)
    {
        printf("当前缓冲区数据长度：%u，继续等待...\n", ringbuf_get_len(rx_ringbuf));
        Sleep(100); // 等待足够的数据到达
    }
    ringbuf_read(rx_ringbuf, data, len);
    printf("成功接收 %u 字节数据\n", len);
}

void MYUSART_ClearRxBuffer(void)
{
    ringbuf_clear(rx_ringbuf);
}

int main()
{
    // 创建环形缓冲区用于接收数据
    rx_ringbuf = ringbuf_create(1024);
    if (rx_ringbuf == NULL)
    {
        printf("错误：无法创建环形缓冲区！\n");
        return -1;
    }

    // 1. 打开串口
    printf("正在打开串口 %s...\n", portName);
    hComm = CreateFile(portName,                     // COM端口名
                       GENERIC_READ | GENERIC_WRITE, // 读写模式
                       0,                            // 独占方式
                       NULL,                         // 安全属性
                       OPEN_EXISTING,                // 打开现有设备
                       FILE_ATTRIBUTE_NORMAL,        // 普通文件属性
                       NULL);                        // 模板文件句柄

    if (hComm == INVALID_HANDLE_VALUE)
    {
        printf("无法打开串口 %s，错误代码: %d\n", portName, GetLastError());

        // 尝试使用\\\\.\\格式打开（适用于COM10及以上）
        if (strlen(portName) > 4)
        {
            char altPortName[20];
            sprintf(altPortName, "\\\\.\\%s", portName);
            hComm = CreateFile(altPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hComm == INVALID_HANDLE_VALUE)
            {
                printf("使用\\\\.\\格式也无法打开串口，错误代码: %d\n", GetLastError());
                return 1;
            }
            printf("使用\\\\.\\格式成功打开串口\n");
        }
        else
        {
            return 1;
        }
    }
    printf("串口打开成功！\n");

    // 2. 获取当前串口参数
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hComm, &dcbSerialParams))
    {
        printf("无法获取串口状态，错误代码: %d\n", GetLastError());
        CloseHandle(hComm);
        return 1;
    }

    // 3. 配置串口参数
    dcbSerialParams.BaudRate = baudRate;   // 波特率
    dcbSerialParams.ByteSize = 8;          // 数据位：8位
    dcbSerialParams.StopBits = ONESTOPBIT; // 停止位：1位
    dcbSerialParams.Parity = NOPARITY;     // 校验位：无

    // 设置流控制（无流控制）
    dcbSerialParams.fBinary = TRUE;
    dcbSerialParams.fParity = FALSE;
    dcbSerialParams.fOutxCtsFlow = FALSE;
    dcbSerialParams.fOutxDsrFlow = FALSE;
    dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
    dcbSerialParams.fInX = FALSE;
    dcbSerialParams.fOutX = FALSE;
    dcbSerialParams.fErrorChar = FALSE;
    dcbSerialParams.fNull = FALSE;
    dcbSerialParams.fAbortOnError = FALSE;

    if (!SetCommState(hComm, &dcbSerialParams))
    {
        printf("无法设置串口参数，错误代码: %d\n", GetLastError());
        CloseHandle(hComm);
        return 1;
    }
    printf("串口参数设置成功：波特率=%d，数据位=8，停止位=1，校验位=无\n", baudRate);

    // 4. 设置超时时间
    timeouts.ReadIntervalTimeout = 50;         // 字符间最大间隔时间(ms)
    timeouts.ReadTotalTimeoutConstant = 50;    // 读取总超时常数
    timeouts.ReadTotalTimeoutMultiplier = 10;  // 读取总超时乘数
    timeouts.WriteTotalTimeoutConstant = 50;   // 写入总超时常数
    timeouts.WriteTotalTimeoutMultiplier = 10; // 写入总超时乘数

    if (!SetCommTimeouts(hComm, &timeouts))
    {
        printf("无法设置超时时间，错误代码: %d\n", GetLastError());
        CloseHandle(hComm);
        return 1;
    }

    // 5. 清空缓冲区
    PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR);
    // -- -创建并启动串口读取线程-- -
    hReadThread = CreateThread(NULL, 0, ReadThreadProc, hComm, 0, NULL);
    if (hReadThread == NULL)
    {
        printf("无法创建读取线程，错误代码: %d\n", GetLastError());
        CloseHandle(hComm);
        return 1;
    }
    printf("串口读取线程已启动。\n");
    printf("开始串口通信，按Ctrl+C退出...\n");

    // 6. 主循环：发送和接收数据
    uint8_t input_time = 4; // 录入次数
    while (1)
    {
        // 接收数据
        printf("请输入操作命令（1-自动验证，2-注册指纹，3-删除指纹，4-清空指纹库）：");
        int c;
        scanf("%d", &c);
        uint8_t id = 0;
        uint8_t ret = 0;
        switch (c)
        {
        case 1:
            ret = PS_AutoIdentify();
            printf("自动验证指纹返回值: %d\n", ret);
            break;
        case 2:
            PS_AutoEnroll(id++, input_time);
            uint8_t cnt = 0;
            do
            {
                cnt = PS_GetEnrollCount(200);
                printf("当前已录入次数: %d\n", cnt);
            } while (cnt < input_time);
            printf("指纹注册完成！\n");
            break;
        case 3:
            PS_DeleteChar(id, 1);
            break;
        case 4:
            PS_Empty();
            break;
        default:
            printf("无效输入，请输入1-5之间的数字。\n");
            continue;
        }

        // PS_AutoIdentify(); // 示例：发送自动验证指纹命令
        memset(receiveBuffer, 0, sizeof(receiveBuffer));
    }

    // 7. 关闭串口
    printf("关闭串口...\n");
    CloseHandle(hComm);

    return 0;
}