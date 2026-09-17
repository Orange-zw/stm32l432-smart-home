# Driver/IO 驱动说明（AI / DI / DO）


## 1. 目录与定位

`Driver/IO/` 包含三类基础驱动：

- `AI`：模拟输入（ADC 采样）
- `DI`：数字输入（GPIO 读电平 + EXTI 中断回调）
- `DO`：数字输出（GPIO 拉高/拉低控制外设）

## 2. 通用设计风格

三个驱动都采用“句柄 + ops 表”的统一风格：

- 句柄：`*_handle_t`，保存端口、引脚、状态等上下文
- 操作表：`*_Operations`，提供 `create/init/destroy/...` 等方法
- 宏调用：`AI_OP(...)`、`DI_OP(...)`、`DO_OP(...)`

推荐调用方式：

1. `*_Create(...)` 创建设备（内部会自动初始化）
2. 周期调用 `read/get/open/close...`
3. 程序退出或设备不再使用时 `*_Destroy(...)`

---

## 3. DO（数字输出）驱动

### 3.1 适用场景

适用于“开/关型”输出控制，如蜂鸣器、继电器、LED 开关等。

项目里的典型场景：

- `template/data_collection.c`：根据告警事件控制蜂鸣器开关
- `template/fall_detection.c`：跌倒/心率异常/闹钟事件触发蜂鸣器
- `HARDWARE/Beep/beep.c`：封装为 `BeepSensor` 的鸣叫接口

### 3.2 关键接口

- 创建/销毁：`DO_Create`、`DO_Destroy`
- 输出控制：`open`、`close`、`toggle`、`set_state`
- 使能控制：`set_enable`、`is_enabled`
- 状态查询：`is_open`

### 3.3 使用示例

```c
DO_handle_t beep = DO_Create(GPIOC, GPIO_Pin_13, IO_HIGH);
if (beep != NULL) {
    DO_OP(beep, open);      // 打开蜂鸣器
    delay_ms(100);
    DO_OP(beep, close);     // 关闭蜂鸣器
}
```

`active_level` 说明：

- `IO_HIGH`：`open` 时输出高电平，`close` 时输出低电平
- `IO_LOW`：`open` 时输出低电平，`close` 时输出高电平（低电平有效外设常用）

### 3.4 注意点

1. `DO_Create` 内部会自动初始化，并默认关闭输出（安全启动）。
2. `set_enable(false)` 会立即把引脚拉到“关闭态”；再 `set_enable(true)` 时会恢复到先前逻辑状态。
3. `DO` 默认 GPIO 模式是**推挽输出** `GPIO_Mode_Out_PP`，若外设需要开漏等模式，需扩展驱动。
4. 当前时钟使能仅覆盖 `GPIOA~GPIOD`，若后续使用 `GPIOE`，要补充分支。

---

## 4. AI（模拟输入）驱动

### 4.1 适用场景

适用于 ADC 电压采样类传感器，如 pH、浊度、TDS、水位等。

使用场景：

- `template/data_collection.c`：  
  - `ph_sensor = AI_Create(GPIOA, GPIO_Pin_5, ADC1)`  
  - `tds_sensor = AI_Create(GPIOA, GPIO_Pin_0, ADC1)`  
  - `turbidity_sensor = AI_Create(GPIOA, GPIO_Pin_1, ADC1)`  
  并通过 `get_raw_value/get_voltage` 进行公式换算


### 4.2 关键接口

- 创建/销毁：`AI_Create`、`AI_Destroy`
- 采样：`get_raw_value`（0~4095）
- 换算：`get_percent_value`（0~100%）、`get_voltage(base_voltage)`（电压值）

### 4.3 使用示例

```c
AI_handle_t ph_sensor = AI_Create(GPIOA, GPIO_Pin_5, ADC1);
if (ph_sensor != NULL) {
    uint16_t raw = AI_OP(ph_sensor, get_raw_value);
    float voltage = AI_OP(ph_sensor, get_voltage, 3.3f);
    (void)raw;
    (void)voltage;
}
```

### 4.4 注意点

1. **支持的引脚映射固定**：当前只映射了 `PA0~PA7`、`PB0~PB1`。若传入未映射引脚，内部通道为 `0xFF`，读取结果会返回 `0`。
2. `ADC1/ADC2` 采用“只初始化一次”逻辑；多传感器共用同一 ADC 是支持的。
3. 采样是**阻塞式**单次转换，放在高实时中断里可能影响实时性。
4. `get_voltage` 的 `base_voltage` 需要与你板子的 ADC 参考电压一致（通常 3.3V）。
5. 驱动只负责电压读取，具体物理量（pH、TDS、浊度）换算和标定应放在业务层。

---

## 5. DI（数字输入）驱动

### 5.1 适用场景

适用于按键、触摸检测、光耦输入、人体触发等数字状态输入。



### 5.2 关键接口

- 创建/销毁：`DI_Create`、`DI_Destroy`
- 读取状态：`read`、`is_active`、`get_active_state`
- 中断功能：`set_callback`（上升沿/下降沿/双边沿）、`enable_irq`

### 5.3 使用示例（轮询方式）

```c
DI_handle_t touch = DI_Create(GPIOA, GPIO_Pin_4, IO_HIGH);
if (touch != NULL) {
    if (DI_OP(touch, is_active)) {
        // 触发逻辑
    }
}
```

### 5.4 使用示例（中断回调方式）

```c
static void touch_cb(DI_handle_t hdi, DI_EventType event, void *args)
{
    (void)hdi;
    (void)event;
    (void)args;
    // 在这里放轻量逻辑（置标志位等）
}

DI_handle_t touch = DI_Create(GPIOA, GPIO_Pin_4, IO_HIGH);
DI_OP(touch, set_callback, DI_EVENT_RISING, touch_cb, NULL);
```

### 5.5 注意点

1. `DI_Create` 会配置 GPIO 输入、EXTI/NVIC，并**默认关闭中断**；`set_callback` 后会**自动开启**。
2. 当前实现里 `DI.c` 的 EXTI 中断服务函数样例被注释了
   **如果不在工程中补齐对应 `EXTI*_IRQHandler` 并调用驱动分发逻辑，回调不会被触发。**  
  
3. GPIO 输入默认先配置为**浮空输入**，再在 EXTI 初始化中改为上拉输入 `GPIO_Mode_IPU`。若硬件是下拉型/外部已有上下拉，需要按电路调整输入模式。
4. 当前时钟使能也只覆盖 `GPIOA~GPIOD`，若使用 `GPIOE` 要补全。
5. 回调运行在中断上下文，避免耗时操作（串口打印、长延时、复杂浮点计算等）。

---


## 6. 常见问题与排查

1. **AI 读数一直是 0**
   - 检查引脚是否在已支持映射中（PA0\~PA7/PB0~PB1）。
   - 检查传感器供电、地线和参考电压。
2. **DI 设置了回调但没有进入**
   - 检查是否补了 `EXTI*_IRQHandler` 并正确清中断标志。
   - 检查引脚上下拉配置和触发边沿是否匹配。
3. **DO 控制反了**
   - 检查 `active_level` 是否与电路一致（高有效/低有效）。
4. **系统卡顿**
   - 检查是否在高频路径中频繁调用阻塞采样（`AI`）或使用长延时蜂鸣。

---

## 8. 初始化顺序

在 `main` 中建议遵循：

1. MCU 时钟与基础外设初始化
2. `DO/AI/DI` 设备 `Create`
3. 上层模块初始化（如 `AS608`、业务状态机）
4. 进入主循环（轮询 + 事件处理）

