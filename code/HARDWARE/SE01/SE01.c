#include "SE01.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include "led.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include <string.h>

// 前置声明：等待确认函数（轮询方式）
static int WaitForConfirmation(uint8_t expected, uint32_t timeout_ms);
#include <string.h>
// 在文件开头定义
char filename[MAX_FILENAME_LENGTH + 1] = {0}; // 定义全局变量
uint8_t currentVolume = 20;                   // 初始音量级别
uint8_t Voice_Level = 20;
// 音量控制命令，涵盖从0级到31级音量
const char *volumeCmds[] = {
    "7E 04 AE 00 B2 7E", // 音量 00
    "7E 04 AE 01 B3 7E", // 音量 01
    "7E 04 AE 02 B4 7E", // 音量 02
    "7E 04 AE 03 B5 7E", // 音量 03
    "7E 04 AE 04 B6 7E", // 音量 04
    "7E 04 AE 05 B7 7E", // 音量 05
    "7E 04 AE 06 B8 7E", // 音量 06
    "7E 04 AE 07 B9 7E", // 音量 07
    "7E 04 AE 08 BA 7E", // 音量 08
    "7E 04 AE 09 BB 7E", // 音量 09
    "7E 04 AE 0A BC 7E", // 音量 10
    "7E 04 AE 0B BD 7E", // 音量 11
    "7E 04 AE 0C BE 7E", // 音量 12
    "7E 04 AE 0D BF 7E", // 音量 13
    "7E 04 AE 0E C0 7E", // 音量 14
    "7E 04 AE 0F C1 7E", // 音量 15
    "7E 04 AE 10 C2 7E", // 音量 16
    "7E 04 AE 11 C3 7E", // 音量 17
    "7E 04 AE 12 C4 7E", // 音量 18
    "7E 04 AE 13 C5 7E", // 音量 19
    "7E 04 AE 14 C6 7E", // 音量 20
    "7E 04 AE 15 C7 7E", // 音量 21
    "7E 04 AE 16 C8 7E", // 音量 22
    "7E 04 AE 17 C9 7E", // 音量 23
    "7E 04 AE 18 CA 7E", // 音量 24
    "7E 04 AE 19 CB 7E", // 音量 25
    "7E 04 AE 1A CC 7E", // 音量 26
    "7E 04 AE 1B CD 7E", // 音量 27
    "7E 04 AE 1C CE 7E", // 音量 28
    "7E 04 AE 1D CF 7E", // 音量 29
    "7E 04 AE 1E D0 7E", // 音量 30
    "7E 04 AE 1F D1 7E"  // 音量 31（最大音量）
};

// 发送读取当前播放文件名的命令
void Read_Current_File(void)
{
    // 发送读取当前文件名的指令
    const char *cmd = "7E 03 CD D0 7E"; // 读取当前播放文件名的指令
    Send_Command(cmd);
    // delay_ms(100);  // 等待语音模块响应（调整延迟以适应模块响应速度）

    //    // 接收并处理返回的数据
    //    Receive_Response();
}

static uint8_t receive_count = 0;

// 处理当前播放文件名的返回值
void HandleCurrentFileResponse(uint8_t *response, uint8_t length)
{
    if (response[0] == PACKET_START)
    {

        int j = 0;

        for (int i = 1; i < length && j < MAX_FILENAME_LENGTH; i++)
        {
            filename[j++] = response[i];
        }

        filename[j] = '\0';
    }
}
void Filename_Clear()
{
    memset(filename, 0, sizeof(filename));
}
void Current_Music()
{
    // OLED_Clear();

    //	OLED_ShowText(0, 2, (uint8_t *)"Current Track:", 0);
    // OLED_ShowText(40, 3, (uint8_t *)filename, 0);
    if (strcmp(filename, "chun.mp3") == 0)
    {
        OLED_ShowText(40, 3, (uint8_t *)"椿", 0);
    }
    else if (strcmp(filename, "gg.mp3") == 0)
    {
        OLED_ShowText(40, 3, (uint8_t *)"Galway Girl", 0);
    }
    else if (strcmp(filename, "spy.mp3") == 0)
    {
        OLED_ShowText(40, 3, (uint8_t *)"ShapeofYou", 0);
    }
    else if (strcmp(filename, "tg.mp3") == 0)
    {
        OLED_ShowText(40, 3, (uint8_t *)"That Girl", 0);
    }
    else if (strcmp(filename, "xrzs.mp3") == 0)
    {
        OLED_ShowText(40, 3, (uint8_t *)"心如止水", 0);
    }
    else if (strcmp(filename, "xtdf.mp3") == 0)
    {
        OLED_ShowText(40, 3, (uint8_t *)"夏天的风", 0);
    }
    else if (strcmp(filename, "ygg.mp3") == 0)
    {
        OLED_ShowText(40, 3, (uint8_t *)"一格格", 0);
    }
    else if (strcmp(filename, "zg.mp3") == 0)
    {
        OLED_ShowText(40, 3, (uint8_t *)"租购", 0);
    }
    else
    {
        OLED_ShowText(40, 3, (uint8_t *)filename, 0);
    }
}
// 设置音量
u8 Set_Volume(uint8_t level)
{
    if (level > 31)
        level = 31;                  // 限制音量范围为0到31
    Send_Command(volumeCmds[level]); // 发送对应的音量控制命令

    Voice_Level = level;
    return level;
}
uint8_t uart_rx_buffer[100]; // 接收缓冲区

// 接收语音模块的返回值
void Receive_Response(void)
{
    //    // 接收语音模块返回的数据
    //    HAL_UART_Receive(&UART3_Handler, uart_rx_buffer, sizeof(uart_rx_buffer), HAL_MAX_DELAY);

    //    // 处理返回的数据
    //    HandleCurrentFileResponse(uart_rx_buffer,);
}
// 发送指令到语音模块
// 将字符串命令转换为实际的十六进制数据并发送
void Send_Command(const char *cmd)
{
    // 串口命令注意事项：命令之间至少间隔 300ms
    delay_ms(300);
    uint8_t hex_cmd[50];
    uint8_t hex_len = 0;

    // 转换字符串为十六进制字节数据
    while (*cmd)
    {
        if (*cmd != ' ') // 跳过空格
        {
            sscanf(cmd, "%2hhx", &hex_cmd[hex_len++]); // 将两个字符转为十六进制字节
            cmd += 2;                                  // 跳过已处理的两个字符
        }
        else
        {
            cmd++; // 跳过空格
        }
    }

    // 使用标准外设库串口发送（USART3）
    Usart_SendString(USART3, (unsigned char *)hex_cmd, hex_len);
}

u8 Receive_command;
// 处理语音模块返回值的具体函数
uint8_t ProcessVoiceResponse(uint8_t response)
{
    // 假设你根据返回值的特定内容做处理，下面是一个简单示例
    if (response == 0x00) // 检查是否返回"OK"
    {
        Receive_command = 0x00;
    }
    else if (response == 0x01) // 检查是否返回"FAIL"
    {
        Receive_command = 0x01;
    }
    else
    {
        Receive_command = 0x02;
    }
}

// 播放文件
void Play_RE003(void)
{
    Send_Command(CMD_PLAY_RE003); // 发送播放命令
                                  //   OLED_ShowText(0, 6, (uint8_t *)"Playing RE003", 0);  // 在OLED上显示播放状态
}

// 停止播放
void PLay_Stop_Music(void)
{
    Send_Command(CMD_Play_STOP); // 发送停止播放命令
}
// 停止播放
void Stop_Music(void)
{
    Send_Command(CMD_STOP); // 发送停止播放命令
}

// 播放下一首
void Next_Track(void)
{
    Send_Command(CMD_NEXT_TRACK);
    //  OLED_ShowText(0, 6, (uint8_t *)"Next Track", 0);
}

// 播放上一首
void Prev_Track(void)
{
    Send_Command(CMD_PREV_TRACK);
    // OLED_ShowText(0, 6, (uint8_t *)"Previous Track", 0);
}
// 删除文件接口
void Delete_File_Interface()
{
    // 发送删除命令
    Delete_File(filename);

    // 等待响应
    if (WaitForConfirmation(0x00, 4000)) // 超时时间为300ms
    {
        OLED_ShowText(50, 6, (uint8_t *)"删除成功", 0);
    }
    else
    {
        OLED_Clear(0);
        OLED_ShowText(0, 3, (uint8_t *)"删除成功", 0);
    }
}

// 串口发送删除命令
void Delete_File(char *filename)
{
    uint8_t cmd[10]; // 7E 07 DB 文件名 校验码 7E
    uint8_t checksum = 0;
    uint8_t i;
    char short_filename[MAX_FILENAME_LENGTH] = {0};

    // 复制文件名，不包括.mp3后缀
    strncpy(short_filename, filename, strlen(filename));
    char *dot = strstr(short_filename, ".");
    if (dot)
    {
        *dot = '\0'; // 截断字符串，移除.mp3后缀
    }

    uint8_t filename_len = strlen(short_filename);

    // 1. 起始码
    cmd[0] = CMD_START;

    // 2. 长度
    cmd[1] = 0x03 + filename_len; // 长度包含命令和文件名的长度

    // 3. 命令 (删除文件命令)
    cmd[2] = CMD_DELETE;

    // 4. 文件名部分（转换为ASCII）
    for (i = 0; i < filename_len; i++)
    {
        cmd[3 + i] = short_filename[i]; // 将不带后缀的文件名转换为ASCII码
    }

    // 5. 校验码计算
    checksum = cmd[1] + cmd[2];
    for (i = 3; i < 3 + filename_len; i++)
    {
        checksum += cmd[i]; // 累加文件名的每个字节
    }
    cmd[3 + filename_len] = checksum & 0xFF; // 取低8位作为校验码

    // 6. 结束码
    cmd[4 + filename_len] = CMD_END;

    // 通过串口发送指令
    Usart_SendString(USART3, (unsigned char *)cmd, 5 + filename_len); // 发送命令数据
}

// 全局状态变量
PlaybackState playbackState = STATE_PLAYING;
// 发送快进命令
void FastForward()
{

    // 切换显示屏至快进界面
    // Display_UpdateScreen(FAST_FORWARD_SCREEN);

    // 更新状态
}

//====================== 标准库通用帧发送与录音接口 ======================

// 构造并发送一帧命令：7E,len,cmd,payload...,checksum,7E
static void SE01_SendFrame(uint8_t cmd, const uint8_t *payload, uint8_t payload_len)
{
    // 串口命令注意事项：命令之间至少间隔 300ms
    delay_ms(300);
    uint8_t frame_len = 5 + payload_len; // 总字节数
    uint8_t frame[5 + 8];                // 预留最多8字节负载（文件名最大8）
    if (payload_len > 8)
        payload_len = 8; // 防御性限制

    frame[0] = CMD_START;
    frame[1] = 0x03 + payload_len; // 长度=命令(1)+校验(1)+负载(payload_len)
    frame[2] = cmd;

    uint8_t checksum = frame[1] + frame[2];
    for (uint8_t i = 0; i < payload_len; i++)
    {
        frame[3 + i] = payload[i];
        checksum += payload[i];
    }

    frame[3 + payload_len] = checksum & 0xFF;
    frame[4 + payload_len] = CMD_END;

    Usart_SendString(USART3, frame, frame_len);
}

// 指定文件名开始录音（文件名最多8字节，自动生成 .MP3）
void SE01_RecordByFileName(const char *name)
{
    // 去除扩展名，仅取前8字节 ASCII
    uint8_t payload[8];
    uint8_t n = 0;
    for (const char *p = name; *p && *p != '.' && n < 8; ++p)
    {
        payload[n++] = (uint8_t)(*p);
    }
    SE01_SendFrame(0xD6, payload, n);
}

// 停止录音（命令 D9）
void SE01_StopRecord(void)
{
    SE01_SendFrame(0xD9, NULL, 0);
}

// 按文件名播放（命令 A3），返回1成功，0失败
int SE01_PlayByFileName(const char *name)
{
    uint8_t payload[8];
    uint8_t n = 0;
    for (const char *p = name; *p && *p != '.' && n < 8; ++p)
    {
        payload[n++] = (uint8_t)(*p);
    }
    SE01_SendFrame(0xA3, payload, n);
    // 大多数模块对播放命令返回 0x00 成功，0x01 失败
    return WaitForConfirmation(0x00, 800) ? 1 : 0;
}

// 读取一次返回码，若超时则返回0x02，并更新 Receive_command
uint8_t SE01_ReadAck(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms)
    {
        if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
        {
            uint8_t data = (uint8_t)USART_ReceiveData(USART3);
            ProcessVoiceResponse(data);
            return data;
        }
        delay_ms(1);
        elapsed++;
    }
    ProcessVoiceResponse(0x02);
    return 0xAA; // 超时/未知
}

// 查询 SD/U 状态（CA）：返回 00/01/02/03，其中 01/03 表示 SD 存在
uint8_t SE01_QuerySD(void)
{
    SE01_SendFrame(0xCA, NULL, 0);
    uint8_t v = SE01_ReadAck(1000);
    if (v == 0xCA) // 有些模块先回操作码
    {
        v = SE01_ReadAck(300);
    }
    return v; // 00 无SD无U；01 有SD无U；02 无SD有U；03 有SD有U
}

// 查询当前工作状态（C2）：01 播放；02 停止；03 暂停；04 录音；05 快进快退
uint8_t SE01_QueryWorkState(void)
{
    SE01_SendFrame(0xC2, NULL, 0);
    uint8_t v = SE01_ReadAck(600);
    if (v == 0xC2)
    {
        v = SE01_ReadAck(300);
    }
    return v;
}

// 语音模块串口初始化封装（PB10/PB11，USART3）
void SE01_Init(u32 baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    // PB10 - USART3_TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // PB11 - USART3_RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART3, &USART_InitStructure);
    USART_Cmd(USART3, ENABLE);
}

// 等待指定确认字节（轮询RXNE），超时返回0，成功返回1
static int WaitForConfirmation(uint8_t expected, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    while (elapsed < timeout_ms)
    {
        if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
        {
            uint8_t data = USART_ReceiveData(USART3);
            if (data == expected)
            {
                return 1;
            }
        }
        delay_ms(1);
        elapsed++;
    }
    return 0;
}