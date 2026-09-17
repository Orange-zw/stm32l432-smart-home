# Driver/UART 驱动说明（`usart_driver`）

## 1. 目录与定位

`Driver/UART/` 目录当前包含：

- `usart_driver.h`
- `usart_driver.c`

`usart_driver` 通用串口驱动层，负责：

- USART1/2/3 的 GPIO + NVIC + 外设初始化
- 中断接收（RXNE + IDLE）
- 环形缓冲区管理
- 统一 ops 接口调用


---

## 2. 统一设计风格

与 IO/PWM 驱动一致，USART 采用“句柄 + ops + 宏”模式：

- 句柄：`USART_handle_t`
- 操作表：`USART_Operations`
- 宏：`USART_OP(dev, op, ...)`

推荐流程：

1. `USART_Create(instance, baud)` 创建设备（内部自动 init）
2. 根据场景选择：
   - 轮询：`is_rx_ready + get_rx_data`
   - 回调：`set_rx_idle_callback`
3. 发送：`send` / `send_fmt`
4. 释放：`USART_Destroy`

---

## 3. 关键接口说明

### 3.1 创建与销毁

- `USART_Create(USART_TypeDef *instance, uint32_t baud_rate)`
- `USART_Destroy(USART_handle_t huart)`

默认配置：

- 8N1（8bit 数据位、1 stop、无校验）
- 收发都开启（`USART_Mode_Rx | USART_Mode_Tx`）
- 开启 `RXNE` 和 `IDLE` 中断

### 3.2 发送接口

- `send(huart, data, length)`：阻塞发送
- `send_fmt(huart, "key=%d", x)`：格式化发送（内部 `vsnprintf`）

### 3.3 接收接口

- `is_rx_ready`：是否有可读数据
- `get_rx_data_length`：当前缓冲区可读长度
- `get_rx_data(buffer, len)`：读取数据（`len=0` 表示取全部）
- `receive`：底层读取实现
- `flush_rx` / `flush_tx`：清空缓冲

### 3.4 回调接口

- `set_rx_byte_callback`：每收到 1 字节触发（RXNE）
- `set_rx_idle_callback`：空闲帧回调（IDLE，一帧完成）
- `set_tx_callback`：发送完成回调

---

## 4. 使用场景

### 4.1 蓝牙参数控制（`template/bt_ref.c`）

使用方式是典型轮询+解析：

```c
bt_uart = USART_Create(USART3, 9600);
if (USART_OP(bt_uart, is_rx_ready)) {
    USART_OP(bt_uart, get_rx_data, rx_data, sizeof(rx_data));
    // 解析 "temp_h=..." / "mode=..." / "fan=..."
    USART_OP(bt_uart, flush_rx);
}
```


### 4.2 LoRa/串口桥接（`template/lcd_ref.c`）

场景与蓝牙类似，区别是既收配置命令又发事件：

```c
lora_uart = USART_Create(USART1, 115200);
USART_OP(lora_uart, send_fmt, "event=%d", EVENT.code);
```


### 4.3 M5310A 云通信（`HARDWARE/M5310A/M5310A.c`）

这里使用了 `set_rx_idle_callback`：

- IDLE 到来时回调把整帧数据拷贝到模块私有缓存
- 上层逻辑通过关键词匹配判断 AT 命令是否成功


### 4.4 AS608 指纹模块（`DEV/AS608/as608.c`）

同样用 `set_rx_idle_callback`，但回调只做“入 ringbuf”，协议解析留在业务函数里。  

---

## 5. 注意点

### 5.1 必须补齐中断入口函数

`usart_driver.c` 里给了 `USART1_IRQHandler/USART2_IRQHandler/USART3_IRQHandler` 示例，但是注释状态。  
如果工程里没有其他地方实现并调用 `USART_IRQHandler(huart)`，那么：

- RXNE/IDLE 中断不会进入驱动
- `is_rx_ready` 可能一直无数据
- 回调不会触发


### 5.2 IDLE 回调后会自动清空 RX 缓冲

在 `USART_IRQHandler` 的 IDLE 分支中，回调执行后会调用 `USART_FlushRx`。  
这意味着 `set_rx_idle_callback` 模式下：

- 回调必须及时拷贝数据（不要只保存指针）
- 回调外不能再依赖驱动 `rx_buffer` 里的旧数据

### 5.3 RX 缓冲写入无溢出保护

`USART_RxBuffer_Write` 采用环形写指针推进，但当前实现未显式检测“写满覆盖”。  
高吞吐场景需在上层加快消费，或扩展驱动增加满缓冲策略。


### 5.4 端口映射当前只实现默认引脚

驱动中 GPIO 初始化固定为：

- USART1：PA9/PA10
- USART2：PA2/PA3
- USART3：PB10/PB11


---

## 6. 接入模板

### 6.1 轮询解析模板（蓝牙/LoRa 类）

```c
USART_handle_t uart = USART_Create(USART3, 9600);
uint8_t rx[128];

if (USART_OP(uart, is_rx_ready)) {
    uint16_t n = USART_OP(uart, get_rx_data, rx, sizeof(rx));
    // parse rx[0..n-1]
    USART_OP(uart, flush_rx);
}
```

### 6.2 IDLE 回调模板（AT 模组/协议帧）

```c
static uint8_t frame_buf[256];
static uint16_t frame_len = 0;

static void uart_idle_cb(uint8_t *data, uint16_t length)
{
    if (length > sizeof(frame_buf)) length = sizeof(frame_buf);
    memcpy(frame_buf, data, length);
    frame_len = length;
}

USART_handle_t uart = USART_Create(USART1, 115200);
USART_OP(uart, set_rx_idle_callback, uart_idle_cb);
```

---

## 7. 常见问题排查

1. **能发不能收**
   - 先确认是否已实现 `USARTx_IRQHandler` 并转发到驱动。
   - 检查 RX 引脚与波特率是否正确。

2. **回调触发了但主循环读不到数据**
   - 使用的是 IDLE 回调模式时，驱动会在回调后 `flush_rx`，需要在回调里拷贝。

3. **偶发丢包/截断**
   - 检查是否接收太快导致环形缓冲覆盖。
   - 检查解析流程是否及时 `get_rx_data`。

4. **格式化发送异常**
   - 检查 `send_fmt` 文本长度是否超 512。
   - 避免在多个上下文并发调用 `send_fmt`。

---
