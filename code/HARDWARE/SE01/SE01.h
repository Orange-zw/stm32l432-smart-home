#ifndef _SE01_H
#define _SE01_H
#include "sys.h"

#define LED1_ON() PCout(13) = 0 // LED1
#define LED1_OFF() PCout(13) = 1
#define LED1_Toggle() PCout(13) = !PCout(13)
#define CMD_START 0x7E        // 起始码
#define CMD_END 0x7E          // 结束码
#define CMD_DELETE 0xDB       // 删除命令
#define CMD_FAST_FORWARD 0xD0 // 快进命令
#define CMD_FAST_REWIND 0xD1  // 快退命令
#define CMD_PLAY_RE003 "7E 08 A3 52 45 30 30 33 D5 7E"
#define CMD_VOLUME_31 "7E 04 AE 1F D1 7E"
#define CMD_NEXT_TRACK "7E 03 AC AF 7E"
#define CMD_PREV_TRACK "7E 03 AD B0 7E"
#define CMD_Play_STOP "7E 03 AA AD 7E"
#define CMD_READ_CURRENT_FILE "7E 03 CD D0 7E"
#define CMD_STOP "7E 03 AB AE 7E"
#define CMD_SEARCH_PLAY "7E 05 A2 00 01 A8 7E"

#define FAST_FORWARD_COMMAND "7E 03 D0 D3 7E"
#define FAST_REWIND_COMMAND "7E 03 D1 D4 7E"

#define ReCord_Start_COMMAND "7E 05 D5 00 14 EE 7E"
#define ReCord_Stop_COMMAND "7E 03 D9 DC 7E"

#define MAX_FILENAME_LENGTH 8
#define PACKET_START 0xCD
#define PACKET_TIMEOUT 50 // 50ms timeout
extern u8 Receive_command;
extern uint8_t currentVolume;
extern uint8_t Voice_Level;
extern char filename[];
typedef enum
{
    STATE_PLAYING,
    STATE_FAST_FORWARD,
    STATE_FAST_REWIND
} PlaybackState;

extern PlaybackState playbackState; // 让其他文件访问该变量
void Receive_Response(void);
void HandleCurrentFileResponse(uint8_t *response, uint8_t length);
void Read_Current_File(void);
uint8_t ProcessVoiceResponse(uint8_t response);
void HandleVoiceModuleData(uint8_t data);
void Prev_Track(void);
// 发送命令到语音模块的函数
void Send_Command(const char *cmd);

// 从语音模块接收数据的函数，并将接收到的内容显示在OLED屏幕上
void Receive_Response(void);

// 播放文件 RE003 的函数
void Play_RE003(void);

// 停止音乐播放的函数
void PLay_Stop_Music(void);

// 设置音量的函数，参数 level 是音量级别（0到31）
u8 Set_Volume(uint8_t level);

// 播放下一首歌曲的函数
void Next_Track(void);

// 播放上一首歌曲的函数
void Prev_Track(void);

void Current_Music();
void Filename_Clear();
void Stop_Music(void);
void Delete_File(char *filename);
// 删除文件接口
void Delete_File_Interface();
void FastForward();
// 录音接口（标准库）
void SE01_RecordByFileName(const char *name);
void SE01_StopRecord(void);
// 按文件名播放（返回1成功，0失败）
int SE01_PlayByFileName(const char *name);
// 读取一次返回码（超时返回0x02）
uint8_t SE01_ReadAck(uint32_t timeout_ms);
// 查询存储介质状态（CA），返回 00/01/02/03；01/03 表示 SD 存在
uint8_t SE01_QuerySD(void);
// 查询当前工作状态（C2），返回 01..05
uint8_t SE01_QueryWorkState(void);
// 串口初始化（PB10/PB11，USART3），传入波特率
void SE01_Init(u32 baudrate);
#endif
