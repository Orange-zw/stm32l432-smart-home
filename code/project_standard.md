## 项目标准

本标准基于 `template` 目录参考例程与当前 `USER/main.c` 归纳，目标是：**所有业务逻辑在 `main.c` 中按统一骨架实现，可快速复用到不同数据采集场景**。

---

## 1) 显示规范

### OLED显示屏
- 尺寸 `128 * 64`
- 字体 size 默认 `16`
- 最多显示 `4` 行

### LCD显示屏
- 尺寸 `160 * 128`
- 字体 size 默认 `16`
- 最多显示 `8` 行

### 页面显示规则
- 时间通常放在数据页第一行，推荐格式 `MM/DD HH:MM:SS`
- 温湿度同屏时优先同一行，推荐格式 `T:35.0C  H:40.0%`
- 页面通常分为：数据页、控制页、设置页（可加测试页）
- 数据页不强制高亮选中项；控制/设置页需要高亮当前选中项
- 页面渲染前先组织 `items`，渲染后清屏并重置 `items_count`

---

## 2) 代码骨架

### 主循环调用顺序
```c
while (1) {
    KEY_Handle();
    SENSOR_Handle();
    AUDIO_Handle();
    APP_Handle();
    EVENT_Handle();
    TIMER_Handle();

    if (MODE == MANUAL) MANUAL_Handle();
    else if (MODE == AUTO) AUTO_Handle();

    PAGES[page_idx]();
}
```

### 中断与低实时任务分离
- 定时器中断中只做轻量动作：`RUN_TIME++`、置位 `TIMER_IT`
- 1秒周期业务（上传、RTC读取、事件时序）放在 `TIMER_Handle()`
- 中断函数只调用 `TIMER_IT_Handle()` 并清中断标志

### 统一函数集合
- `Switch_Mode`, `SAVE_config`, `LOAD_config`
- `MANUAL_Handle`, `AUTO_Handle`, `EVENT_Handle`
- `SENSOR_Handle`, `APP_Handle`, `AUDIO_Handle`
- `KEY_Handle`, `TIMER_Handle`, `TIMER_IT_Handle`

---

## 3) 数据结构与命名规范

### Mode
- 使用 `enum Mode { MANUAL, AUTO, MODE_MAX }`
- 提供 `MODE_STR[MODE_MAX]` 方便显示

### Data
- `DATA` 存放采集值 + 必要状态量（如 GPS、姿态、计时状态）
- 字段名采用业务语义，避免缩写歧义

### Config
- `CONFIG` 仅存放需要掉电保存的参数
- 阈值命名统一：`{name}_h` / `{name}_l`
- 推荐对 `CONFIG` 增加 `flag` 校验字段（如 `CONFIG_FLAG`）

### Event
- 事件枚举统一以 `EVENT_` 前缀命名
- 必须有 `EVENT_NORMAL` 与 `EVENT_CODE_MAX`
- 推荐结构：
  - `cur` / `last`（或 `code` / `last_code`）用于边沿触发
  - 事件描述字符串
  - 是否允许上传 `isUpload`

---

## 4) 按键与页面规范

### 按键定义
- 固定宏：
  - `BTN_PAGE_SWITCH`：切页
  - `BTN_ITEM_SWITCH`：切换当前项
  - `BTN_UP` / `BTN_DOWN`：参数增减或开关控制

### 页面数组
- 统一入口：`void (*PAGES[])(void)`
- 至少包含：`Page_data*` + `Page_control`/`Page_settings`
- `page_count = sizeof(PAGES) / sizeof(PAGES[0])`

### 页面数据缓存
- 页面函数流程
```
// 按键逻辑

// 填充显示项
item_idx_min = 0;
items_count = 0;
sprintf((char *)items[items_count++], "状态: %s", EVENT.cur.str);
```

---

## 5) 事件处理

- `AUTO_Handle()` 中按优先级做单事件判定（`if -> else if -> else`）
- `EVENT_Handle()` 只在事件变化时触发动作与上报
- 事件切换分两段：
  1. 处理上一个事件的善后（关闭设备、状态回收）
  2. 处理当前事件动作（报警、执行器动作）

---

## 6) 数据采集与驱动规范

- 优先使用 `Driver` 目录驱动；可实现则不要回退 `HARDWARE` 版本
- 数字/模拟输入输出优先走 `IO_sensor` 抽象接口
- 采样结果建议做有效性过滤（量程判断、无效值抑制）
- 业务计算统一封装为独立函数（如 `PH_Check` / `Current_IN`）

---

## 7) 通信与云端协议规范

### 接口选型
- ESP8266 使用现有库
- 其他串口模块统一走 `USART_driver`
- 常用波特率：
  - Bluetooth `9600`
  - LORA `115200`
  - NB-IOT `9600`

### 命令解析
- 在 `APP_Handle()` 中统一处理
- 阈值设置命令直接使用变量名：如 `temp_h=35`、`tds_l=200`
- 模式切换统一 `mode=<num>`
- 每次参数变更后调用 `SAVE_config()`，可配合短蜂鸣反馈

### 上报格式
- 设备数据建议以 `#...#dev_data#` 结尾
- 事件数据建议以 `$<code>$event_data$` 传输
- 上传内容至少包含：关键传感器数据 + 主要执行器状态 + 当前模式

---

## 8) Flash 配置

- `SAVE_config()` 必做约束：
  - 各阈值上下限 `constrain`
  - 关联阈值保持一致性（如 `low <= high`）


---

## 9) 实现约束

- 项目代码统一在 `main.c`，不拆分业务到额外 `.c`
- 允许新增辅助函数，但保持“初始化 -> 主循环 -> 各 Handle”结构
- 命名、流程、页面组织尽量与 `template` 参考例程一致
- 新项目优先复用模板中的函数布局和调用顺序

---
