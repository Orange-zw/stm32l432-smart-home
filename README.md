# STM32 智能家居控制系统（离线语音 + 云端远程）

基于 **STM32F103C8T6** 的智能家居控制系统，提供 **按键 / 离线语音 / 手机 APP 远程** 三个控制入口，
控制台灯、客厅灯、风扇三档调速、空调模式、窗户开关五类执行器，
配 OLED 分页人机界面，参数掉电保存，并通过 ESP8266 接入巴法云实现公网远程控制。

> 本仓库内容原为 STM32L432 + uC/OS-III 方案，现已整体替换为本项目（STM32F103C8T6 裸机方案）。
> 旧版本仍可通过 Git 历史找回。

---

## 一、项目概览

| 项目 | 说明 |
|---|---|
| 主控 | STM32F103C8T6（Cortex-M3 @72 MHz，64 KB Flash / 20 KB SRAM，LQFP48） |
| 本地交互 | 4 个独立按键 + 0.96" OLED（SSD1306，128×64，软件 I2C） |
| 语音交互 | SU-03T 离线语音识别模块（UART 9600 8N1），14 条命令词 |
| 云端交互 | ESP8266（ESP-01S，UART 115200）→ 巴法云 bemfa.com → 手机 APP |
| 执行器 | 台灯 LED、客厅灯 LED、风扇（三档 PWM 调速）、空调（制热/制冷指示）、窗户（SG90 舵机） |
| 提示 | 有源蜂鸣器（PC13，MOS 管驱动） |
| 掉电保存 | 片内 Flash 最后一页（0x0800FC00），魔数 0x12345678 校验 |
| 开发环境 | Keil MDK5（ARM Compiler 5，C99）、STM32 标准外设库（SPL，非 HAL） |
| 资源占用 | Flash 34.8 KB / 64 KB（54.5%）、RAM 10.4 KB / 20 KB（52.3%）、最大栈 360 B |

### 语音能力的准确边界

**真正做语音识别的地方只有 SU-03T 这一颗芯片，全部在设备端离线完成；云平台不做语音识别。**
巴法云在本项目中的定位是 **IoT 消息中转平台（broker）**，只负责收发字符串报文。

| 路径 | 语音识别在哪 | 是否需要网络 | 本项目状态 |
|---|---|---|---|
| ① 离线语音（主路径） | SU-03T 芯片内部 | 不需要 | 已实现 14 条命令词 |
| ② 手机 APP 语音输入 | 手机端 ASR | 需要 | 取决于 APP，固件侧只接收字符串命令 |
| ③ 智能音箱（小爱/小度/天猫精灵） | 音箱云端 ASR | 需要 | 未对接（巴法云原生支持） |

---

## 二、功能列表

1. 配备 SU-03T 语音模块，支持语音识别和控制功能
2. 配备 DRV8833 电机驱动模块，控制风扇三档调速
3. 配备白色 LED 模块（台灯）与绿色 LED 模块（客厅灯）
4. 配备 SG90 舵机模拟开关窗户
5. OLED 显示屏实时显示工作模式、各设备开关状态、WiFi 连接状态
6. 通过按键切换手动与语音控制模式
7. 手动模式下通过按键控制台灯、客厅灯、风扇三档调速、空调模式切换、舵机开关窗户
8. 语音模式下通过语音指令实现上述同样的控制
9. 接收到远程设备指令时自动发出确认信息
10. 通过 WiFi 模块与手机 APP 实现远程控制设备开关和模式切换
11. 手机 APP 远程切换手动/语音控制模式
12. 识别特定语音场景（如"我要睡觉了""我要出门了"）时自动执行相应设备控制

---

## 三、硬件设计

### 引脚分配

| 功能 | 引脚 | 配置 | 代码位置 |
|---|---|---|---|
| OLED SCL | PB8 | 开漏输出（软件 I2C） | `main.c: oled_init()` |
| OLED SDA | PB9 | 开漏输出 | 同上 |
| KEY1 | PA12 | 上拉输入 | `Key.c` |
| KEY2 | PA15 | 上拉输入 | `Key.c` |
| KEY3 | PB3 | 上拉输入（需关闭 JTAG） | `Key.c` |
| KEY4 | PB4 | 上拉输入（需关闭 JTAG） | `Key.c` |
| 蜂鸣器 | PC13 | 推挽输出，高电平有效 | `DO_Create(GPIOC, Pin_13, IO_HIGH)` |
| 台灯 LED（白） | PB12 | 推挽输出，低电平点亮 | `DO_Create(GPIOB, Pin_12, IO_LOW)` |
| 客厅灯 LED（绿） | PB13 | 推挽输出，低电平点亮 | `DO_Create(GPIOB, Pin_13, IO_LOW)` |
| 空调制冷指示（蓝） | PB14 | 推挽输出，低电平点亮 | `DO_Create(GPIOB, Pin_14, IO_LOW)` |
| 空调制热指示（红） | PB15 | 推挽输出，低电平点亮 | `DO_Create(GPIOB, Pin_15, IO_LOW)` |
| 风扇调速 | PA8 | 复用推挽，TIM1_CH1，1 kHz PWM | `PWM_Create(GPIOA, Pin_8)` |
| 窗户舵机 | PA6 | 复用推挽，TIM3_CH1 | `SG90_Create(GPIOA, Pin_6, 0)` |
| SU-03T TX→MCU | PA10 | 浮空输入（USART1） | `su_03t.c` |
| SU-03T RX←MCU | PA9 | 复用推挽 | 同上 |
| ESP8266 TX→MCU | PA3 | 浮空输入（USART2） | `esp8266.h: Bamfa_USART` |
| ESP8266 RX←MCU | PA2 | 复用推挽 | 同上 |
| 系统 1 ms 心跳 | TIM2（无引脚） | PSC=72 / ARR=1000 / 1 kHz 更新中断 | `Timer_Init(72, 1000)` |

### 外设资源划分

```
                    ┌──────────────────────────────────────┐
   SU-03T 语音 ──USART1(PA9/PA10, 9600)──►│                      │
                    │    STM32F103C8T6     │──TIM3_CH1(PA6)──► SG90 舵机（窗户）
   ESP8266 WIFI ─USART2(PA2/PA3,115200)─►│    72MHz / 64K/20K   │──TIM1_CH1(PA8)──► 风扇调速
                    │                      │──PB12/13/14/15──────► 4 路 LED
   KEY1..KEY4 ───PA12/PA15/PB3/PB4──────►│                      │──PC13────────────► 蜂鸣器
   OLED(SSD1306) ─软I2C(PB8=SCL,PB9=SDA)►│                      │
                    └──────────────────────────────────────┘
```

### 原理图

`原理图PCB/Schematic_26B0482基于物联网的语音识别系统.pdf`

> ⚠️ **已知不一致（需实物核对）**：原理图中风机经 **DRV8833 双 H 桥**（AIN1/AIN2）驱动，
> 而当前固件风机走的是 **PA8 的 TIM1_CH1 单线 PWM 调速**。两者不一致，交付前需用万用表/示波器核对
> PA8 与 AIN1/AIN2 的实际接线，并据此统一硬件或固件。

---

## 四、软件架构

### 分层

```
命令解析层   KEY_Handle / AUDIO_Handle(SU-03T) / APP_Handle(ESP8266)
                          ↓ 三入口统一收敛
业务语义层   fan_control / fan_set_gear / AC_control / Door_control / Switch_Mode
                          ↓
设备驱动层   DO(数字输出) / PWM / SG90 / SSD1306 / STMFLASH
                          ↓
抽象与工具   Driver/IO · Utils(utility.h / Ring_Buf / List / filter-c)
```

### 核心设计模式

**1. Handle + Operations（句柄 + 操作表）**

每个设备用一个结构体承载全部状态，用一组函数指针定义"这类设备能做什么"，
业务层通过 `DO_OP(dev, op, ...)` 统一调用：

```c
DO_handle_t beep = DO_Create(GPIOC, GPIO_Pin_13, IO_HIGH);
DO_OP(beep, open);
DO_OP(led_white, set_state, 1);
```

好处：硬件细节（端口/引脚/有效电平）被隔离在一行 `Create` 里，业务层不出现任何寄存器操作；
低电平点亮的 LED 与高电平有效的蜂鸣器在业务层语义统一为 `open`/`close`。

**2. 中断高/低实时分离**

TIM2 配成 1 kHz，中断里只做 `RUN_TIME++` 与置标志位（微秒级）；
秒级业务（云端上报）交给主循环的 `TIMER_Handle()`：

```c
void TIM2_IRQHandler(void) {          /* 高实时：极短 */
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
        TIMER_IT_Handle();
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
void TIMER_Handle(void) {             /* 低实时：主循环 */
    if (TIMER_IT == 0) return;
    TIMER_IT = 0;
    /* 每秒向巴法云上报设备状态 */
}
```

**3. 页面表 + 条目框架**

```c
void (*PAGES[])(void) = { Page_control, Page_settings };
uint8_t page_count = sizeof(PAGES) / sizeof(PAGES[0]);
```

页面函数固定三段式：**按键处理 → 组织条目 → 渲染（含滚动窗口与反白高亮）**。
新增页面只需写一个函数并加进 `PAGES[]`。

**4. 查表代替分支**

```c
const uint8_t  FAN_DUTY_LEVEL[4] = {0, 35, 70, 100};      /* 档位 → 占空比 */
const char    *FAN_LEVEL_STR[4]  = {"0档","1档","2档","3档"};
const char    *AC_STATE_STR[3]   = {"关闭","制热","制冷"};
```

档位合法性由 `_constrain(gear, 0, 3)` 保证，因此查表永不越界。

### 主循环骨架

```c
while (1) {
    KEY_Handle();        /* 按键扫描 + 页/项切换 */
    SENSOR_Handle();     /* 传感器处理（本版本为空：本板无传感器） */
    AUDIO_Handle();      /* 离线语音命令解析 */
    APP_Handle();        /* 云端命令解析 */
    TIMER_Handle();      /* 1 秒软定时：云端上报 */
    if (MODE == MANUAL) MANUAL_Handle();
    PAGES[page_idx]();   /* 当前页面渲染 */
}
```

---

## 五、通信协议

### 1. SU-03T 离线语音（USART1，9600 8N1）

帧格式（模块主动上报）：

```
0xAA 0x55 CMD 0x55 0xAA
 帧头    命令号   帧尾
```

命令号映射（0x01 ~ 0x0E）：

| 命令 | 功能 | 命令 | 功能 |
|---|---|---|---|
| 0x01 | 打开台灯 | 0x08 | 空调制热 |
| 0x02 | 关闭台灯 | 0x09 | 空调制冷 |
| 0x03 | 打开客厅灯 | 0x0A | 关闭空调 |
| 0x04 | 关闭客厅灯 | 0x0B | 打开窗户 |
| 0x05 | 风扇加速 | 0x0C | 关闭窗户 |
| 0x06 | 风扇减速 | 0x0D | 关闭所有电器 |
| 0x07 | 关闭风扇 | 0x0E | 关闭所有灯光 |

接收实现（`su_03t.c` + `AUDIO_Handle()`）采用**双路径解析**：
标准帧路径（校验头尾后取中间 0x01~0x0E）为主，**裸单字节兜底路径**兼容不同模组固件的输出差异。
ISR 内设有"缓冲区还有未消费帧就丢弃新字节"的**背压保护**，保证主循环读到的缓冲区不被并发改写。

### 2. ESP8266 + 巴法云（USART2，115200）

| 方向 | 报文 |
|---|---|
| 连接 | `AT+CWJAP="SSID","PWD"` → `AT+CIPSTART="TCP","bemfa.com",8344` |
| 订阅 | `cmd=1&uid=<私钥>&topic=control` |
| 设备上报（1 s 周期） | `cmd=2&uid=<私钥>&topic=data&msg=#<模式>#<台灯>#<窗户>#<客厅灯>#<蓝>#<红>#<风扇档>#dev_data#` |
| APP 下发 | `led_white=1` / `led_green=0` / `led_blue=0` / `led_red=0` / `door=1` / `mode=0|1` / `fan_speed=1|0` |

`Flagout == 0` 表示已连接云端，仅此时才上报。

---

## 六、目录结构

```
.
├── code/                        # 固件工程（Keil）
│   ├── USER/                    # 业务层：main.c、中断向量、Keil 工程 LCD.uvprojx
│   ├── Driver/                  # 通用外设驱动（与具体器件无关）
│   │   ├── IO/                  #   DO（数字输出）/ DI（数字输入）/ AI（模拟输入）抽象
│   │   ├── PWM/                 #   PWM 驱动 + GPIO→TIMx_CHy 映射表
│   │   ├── UART/ SOFT_I2C/ HARD_I2C/ ONE_WIRE/ TIM_IC/
│   ├── DEV/                     # 具体器件驱动
│   │   ├── OLED_new/            #   SSD1306（含 GB2312 字库）
│   │   ├── SG90/                #   SG90 舵机（内部组合 PWM 驱动）
│   │   └── DS18B20/
│   ├── HARDWARE/                # 各模块驱动（本项目实际使用其中一部分）
│   │   ├── SU_03T/  ESP8266/  KEY/  Timer/  STMFLASH/  OLED/
│   │   └── …（还有 GPS、RC522、MPU6050、VL53L0X 等 60+ 个未编译模块）
│   ├── Utils/                   # utility.h（通用宏/位操作）+ Ring_Buf + List + filter-c + lwgps
│   ├── SYSTEM/                  # delay（延时）、sys（位带操作）
│   ├── CORE/                    # Cortex-M3 内核 + 启动文件
│   ├── STM32F10x_FWLib/         # ST 标准外设库源码
│   ├── CONNECT/                 # ESP8266.lib（预编译静态库）
│   ├── NEAI/                    # ST NanoEdgeAI 边缘 AI 静态库（本项目未链接）
│   ├── Third_Party/             # FatFs
│   ├── OBJ/                     # 构建产物（LCD.hex 可烧录 + 库打包脚本）
│   └── project_standard.md      # 工程编码规范
├── app/B0482.apk                # 手机 APP
├── jx_firm/                     # SU-03T 模组固件（烧录器版 / 串口升级版）+ 协议说明
└── 原理图PCB/                    # 原理图 PDF
```

---

## 七、在线调试与烧录配置

| 项 | 值 |
|---|---|
| 目标器件 | STM32F103C8（IROM 0x08000000 / 0x10000，IRAM 0x20000000 / 0x5000） |
| 系统时钟 | 72 MHz（HSE 8 MHz × PLL9），`SystemInit()` 配置 |
| 编译器 | ARM Compiler 5（AC5），C99，优化 Level 3（`<Optim>4`） |
| 编译选项 | `--no-multibyte-chars`、`USE_STDPERIPH_DRIVER` |
| 输出 | `code/OBJ/LCD.hex` |
| 下载方式 | ST-Link / J-Link（SWD，PDI 保留 SWD、关闭 JTAG 以释放 PB3/PB4） |

### ⚠️ 构建须知：源码编码约定

`code/USER/main.c`、`code/HARDWARE/ESP8266/esp8266.h`、`code/HARDWARE/OLED/oledfont.h` 等
文件是 **GB2312 编码**，且必须保持 GB2312 —— 它们与 `DEV/OLED_new/ssd1306_fonts.c` 里的
**GB2312 点阵字库**配套使用，Keil 也配置了 `--no-multibyte-chars`。

- Keil 中请将 Encoding 设为 **Chinese GB2312**
- **不要**对这些文件批量执行转 UTF-8（会破坏中文字符串与字库索引，导致 OLED 中文乱码）
- 仓库已用 `.gitattributes` 将这批文件标记为 `binary`，避免 EOL/编码被自动转换

---

## 八、资源占用基线

| 区域 | 占用 | 说明 |
|---|---|---|
| Flash | 34 800 B / 65 536 B（54.5%） | 其中 GB2312 字库单项占 17 299 B |
| RAM | 10 400 B / 20 480 B（52.3%） | 其中 `items[10][256]` 行缓冲占 2 560 B、OLED 显存 1 024 B |
| 最大栈深度 | 360 B | + 函数指针调用等不可追踪部分 |
| 配置存储 | `0x0800FC00`（最后一页，1 KB） | 使用上限红线 0xFC00，固件不得涨过 63 KB |

---

## 九、已知问题与后续改进方向

按优先级排列：

### 可靠性
- [ ] **未启用独立看门狗（IWDG）** —— 现场设备应有，否则一次 HardFault 即永久死机
- [ ] **WiFi 无断线检测与重连** —— 巴法云心跳指令 `cmd=7&uid=...&type=1` 已定义但未启用
- [ ] **协议无校验** —— SU-03T 帧与云端明文键值对均无 CRC，建议加 CRC8
- [ ] `APP_Handle()` 中 `sscanf` 返回值未校验，收到 `led_white=abc` 会使用未初始化变量
- [ ] `AUDIO_Handle()` 的"丢弃新帧"背压策略会丢命令，应改为环形缓冲 + 读写指针

### 实时性
- [ ] `KEY_Scan()` 的 `delay_ms(10)` 去抖与 `Beep_Beep()` 的阻塞延时改为定时器状态机
- [ ] `ESP8266_SendData()` 阻塞发送改为 DMA + 完成中断

### 资源
- [ ] `items[10][256]` 冗余约 2 KB RAM，应改用驱动已提供的 `oled_draw_text_line_fmt()`
- [ ] GB2312 字库 17.3 KB 可做子集化裁剪
- [ ] `SAVE_config()` 在每次按键时都擦写 Flash 且内容不变，应加脏标记/磨损均衡

### 安全
- [ ] WiFi 密码与巴法云私钥硬编码在 `esp8266.h`，应改为首次配网下发 + 一机一密

### 功能
- [ ] `SENSOR_Handle()` 为空，自动模式（阈值联动）与事件上报未启用
- [ ] SU-03T 语音播报应答（`SU_03T_SendArray()`）已写好接口但业务层未调用

---

## 十、第三方代码说明

以下目录为**已收录进本仓库的第三方代码**（其独立的 `.git` 目录已剥离，以便随仓库一起分发）：

| 目录 | 来源 | 说明 |
|---|---|---|
| `code/Utils/filter-c/` | [adis300/filter-c](https://github.com/adis300/filter-c) | C 语言滤波器库 |
| `code/Utils/List/` | [SunyanDSB/Embedded-List](https://github.com/SunyanDSB/Embedded-List) | 嵌入式链表库 |
| `code/keil2clangd-master/` | keil2clangd | Keil 工程转 clangd 配置（辅助工具源码，可删除） |
| `code/Third_Party/FatFs/` | ChaN FatFs | 文件系统 |
| `code/NEAI/` | ST NanoEdgeAI Studio | 边缘 AI 静态库（本项目未链接） |
| `code/CONNECT/ESP8266.lib` | 本项目自制 | 由 `code/OBJ/A_make_esp8266_lib(2).bat` 用 `armar` 打包 |

---

## License

本仓库代码仅供学习与交流使用。第三方组件版权归各自作者所有。
