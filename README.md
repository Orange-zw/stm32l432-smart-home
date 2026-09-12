# 基于 STM32L432 的智能家居控制系统

> 一个 **STM32L432 + uC/OS-III** 的智能家居终端：本地多传感器采集与自动控制，经 **ESP8266 WiFi** 上报云端，并由 **Linux 网关 + Android APP** 实现远程监控与联动。

---

## 1. 系统架构

```
┌──────────────────────────┐        ┌───────────────────────────┐        ┌──────────────────┐
│   STM32L432 智能家居终端  │        │      Linux 网关服务        │        │   Android APP    │
│                          │  TCP   │        (server.c)         │  TCP   │  SmartControl    │
│  uC/OS-III 多任务         │◄──────►│  父进程 :9999  ←→  硬件    │◄──────►│                  │
│  ├ 数据采集任务           │  9999  │  子进程 :8001  ←→  手机APP │  8001  │  实时数据显示     │
│  ├ 电器控制任务           │        │  管道 IPC + 多线程 + 心跳  │        │  远程指令下发     │
│  └ 命令解析任务           │        └───────────────────────────┘        └──────────────────┘
│                          │
│  传感器：DS18B20 / MQ-2 / MQ-135 / GP2Y1050(PM2.5)
│  执行器：空气净化器 / 抽风机 / 照明
│  通信：ESP8266 (AT 指令, TCP Client)
└──────────────────────────┘
```

## 2. 硬件组成

| 模块 | 器件 / 接口 | 说明 |
|---|---|---|
| 主控 | **STM32L432**（ARM Cortex-M4，超低功耗 L4 系列） | 系统主控，跑 uC/OS-III |
| 温度采集 | **DS18B20**（GPIO 位操作模拟 1-Wire） | 微秒级时序驱动，精度 0.0625℃ |
| 可燃气体/烟雾 | **MQ-2** → ADC1_CH15 | 12 位 ADC 采集，多次平均滤波 |
| 空气质量 | **MQ-135** → ADC1_CH16 | 电压分档判定 A/B/C 等级 |
| PM2.5 | **GP2Y1050** → USART2 @2400bps | 自定义帧协议（0xAA 帧头 + 校验和） |
| 无线上报 | **ESP8266** → USART1 | AT 指令驱动，STA 模式 TCP 长连接 |
| 定时器 | **TIM7** | 作为串口帧超时判据，判定一帧接收完成 |
| 执行器 | 空气净化器 / 抽风机 / 照明 | GPIO 输出，支持手动与自动两种模式 |

## 3. 软件设计要点

### 3.1 uC/OS-III 多任务划分
| 任务 | 优先级 | 职责 |
|---|---|---|
| `start_task` | 3 | 系统初始化并创建其余任务后挂起 |
| `led0_task` | 4 | 电器控制：周期调用 `Control()` 执行手动/自动控制逻辑 |
| `led1_task` | 5 | 数据采集与上报：采集各传感器并组帧上报服务器 |
| `float_task` | 6 | 远程命令解析：扫描并解析云端下行指令 |

- 使用 **临界区**（`OS_CRITICAL_ENTER/EXIT`）保护 DS18B20 这类对时序敏感、不可被打断的代码段；
- 中断服务程序中使用 `OSIntEnter()/OSIntExit()` 与 RTOS 正确协作；
- 使能 **时间片轮转调度**（`OSSchedRoundRobinCfg`）并配置各任务独立堆栈。

### 3.2 传感器驱动实现
- **DS18B20**：纯 GPIO 位操作实现复位脉冲、读写时隙（`delay_us` 级时序），完成温度转换与负温处理；
- **GP2Y1050（PM2.5）**：自定义串口帧解析——以 `0xAA` 为帧头，接收 5 字节并做 **校验和验证**（`buf[4] == buf[0..3] 之和`），累计多次采样求平均后换算浓度；**采集完成后自动关闭 RX 中断**，降低 CPU 开销；
- **ADC**：MQ-2/MQ-135 多次采样取平均以抑制噪声，再按电压档位判定空气等级。

### 3.3 通信协议与远程控制
- 通过 AT 指令序列（`ATE0` / `CWMODE` / `CIPMUX` / `CWJAP` / `CIPSTART` / `CIPSENDEX`）完成 ESP8266 联网与 TCP 连接，并用 **状态机**（位域 `ESP8266_STA`）管理连接与收包过程；
- 自定义数据帧上报传感器数据：
  ```
  {0,<MQ2>,<MQ135>,0,0,<温度>°C,<手动状态>,<自动状态>,0,<灯>,<净化器>,<抽风机>,0,0,0,0,<PM2.5等级>}
  ```
- 下行命令由 `Cmd_Scan()` 解析，提取手动/自动控制位，实现远程开关设备。

### 3.4 手动 / 自动控制策略
- 以**位域**维护设备状态：`bit7 空调 · bit6 空气净化器 · bit5 抽风机 · bit4 窗 · bit3 灯`；
- **手动优先级高于自动**，两者都关闭时设备才完全关闭；
- 自动模式下依据空气质量等级（A/B/C）联动开启净化器与抽风机。

## 4. Linux 网关服务（`server.c`）

- **多进程**：`fork()` 分离父子进程，父进程监听 **9999** 对接硬件终端，子进程监听 **8001** 对接手机 APP；
- **进程间通信**：双向 `pipe()` 在两个进程间转发数据；
- **多线程**：接收线程、管道读取线程、心跳清理线程分别处理收包与超时；
- **心跳超时机制**：每 3s 计数一次，连续 7 次（**21s**）无数据则主动断开失效连接，避免连接泄漏；
- 编译方式：`gcc -o server server.c -lpthread`

## 5. Android 客户端

`SmartControl`：基于 TCP Socket 与网关通信，实时显示温度、空气质量、PM2.5 等数据，并下发开关指令控制家电。

## 6. 目录结构

```
.
├── STM32L432/               # 固件工程（Keil MDK）
│   ├── USER/                # main.c 与工程配置
│   ├── HARDWARE/            # 自研驱动：DS18B20 / esp8266 / adc / led / timer / usart2
│   ├── UCOSIII/             # uC/OS-III 内核与移植（含 uC-CPU / uC-LIB / BSP / 配置）
│   ├── SYSTEM/              # delay / sys / usart 基础支持
│   ├── CORE/                # CMSIS 内核与启动文件
│   └── HALLIB/              # STM32L4xx HAL 库
├── server.c                 # Linux 网关服务
└── android/SmartControl/    # Android 客户端源码（Eclipse ADT 工程）
    ├── src/com/example/smartcontrol/   # MainActivity / SettingsActivity
    ├── res/                            # 布局、控件、图标资源
    └── AndroidManifest.xml
```

## 7. 编译与运行

**固件（Keil MDK）**
1. 打开 `STM32L432/USER/UCOSIII.uvprojx`；
2. 编译并下载到 STM32L432 开发板；
3. 在 `HARDWARE/src/esp8266.c` 中修改 WiFi 账号密码与服务器 IP / 端口。

**网关（Ubuntu / Linux）**
```bash
gcc -o server server.c -lpthread
./server
```

**手机端**：安装 `SmartControl`，在设置页填入网关 IP 与端口（8001）。

## 8. 说明

- 本仓库固件基于 **uC/OS-III** 实时内核；HAL 库与 uC/OS-III 内核为 ST / Micrium 官方源码。
- 自研部分集中在 `HARDWARE/`（传感器与 WiFi 驱动）、`USER/main.c`（任务设计与控制逻辑）、`server.c`（网关）与 Android 客户端。
