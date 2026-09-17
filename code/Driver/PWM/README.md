# Driver/PWM 驱动说明（`pwm_driver`）


## 1. 目录与定位

`Driver/PWM/` 当前包含：

- `pwm_driver.h`
- `pwm_driver.c`

用于“可移植 PWM 输出”的通用驱动层，支持：

- 多定时器与多通道映射（含重映射）
- 占空比设置
- 频率设置 / 预分频+周期配置
- 启停控制与状态查询


---

## 2. 驱动设计风格

与 IO 驱动一致，PWM 也使用“句柄 + ops 表 + 宏调用”：

- 句柄：`PWM_Handle_t`
- 操作表：`PWM_Operations`
- 宏：`PWM_OP(dev, op, ...)`

典型生命周期：

1. `PWM_Create(gpio_port, gpio_pin)`：创建并自动初始化硬件
2. `PWM_OP(..., setDutyCycle, duty)`：设置占空比
3. （可选）`setFrequency` 或 `setConfig`
4. 不再使用时 `PWM_Destroy`

---

## 3. 核心接口说明

### 3.1 创建与销毁

- `PWM_Create(GPIO_TypeDef *gpio_port, uint16_t gpio_pin)`
- `PWM_Destroy(PWM_Handle_t *driver)`

`PWM_Create` 成功后会：

- 完成 `GPIO/Timer/Channel` 初始化
- 默认配置 `PSC=71`、`ARR=999`（1kHz）
- 默认占空比 `0%`
- **自动调用 `start` 启动输出**

### 3.2 控制接口

- `start` / `stop`
- `setDutyCycle(float duty_cycle)`：设置占空比（百分比）
- `setFrequency(uint32_t frequency)`：设置目标频率（内部固定 `ARR=999`）
- `setConfig(uint32_t prescaler, uint32_t period)`：直接设定 PSC/ARR

### 3.3 查询接口

- `isEnabled`
- `getDuty`
- `getFrequency`
- `getPrescaler`（返回 `PSC+1`）
- `getPeriod`（返回 `ARR+1`）

---

## 4. 使用场景

### 4.1 风扇调速（`template/bt_ref.c`）

场景：通过档位数组控制风扇转速（0/低/中/高档）。

典型流程：

```c
fan = PWM_Create(GPIOA, GPIO_Pin_6);
PWM_OP(fan, setDutyCycle, DATA.fan_gear_value[DATA.fan_gear]);
```

说明：

- `PA6` 在映射表中对应 `TIM3_CH1`，适合普通 PWM 输出。
- 自动模式下会根据 CO2 / 烟雾浓度动态切换档位并更新占空比。

### 4.2 LED 亮度调节（`template/fall_detection.c`）

场景：按档位调节 LED 亮度。

典型流程：

```c
led = PWM_Create(GPIOA, GPIO_Pin_7);
PWM_OP(led, setDutyCycle, 0);      // 上电先熄灭
PWM_OP(led, setDutyCycle, 30);     // 低亮度
```

说明：

- 当前项目使用 `PA7`，驱动会按映射表优先匹配一个 PWM 通道（见“注意点”章节中的“重复引脚映射优先级”）。

### 4.3 SG90 舵机控制（`DEV/SG90/sg_90.c`）

场景：舵机需要 50Hz 周期脉冲，脉宽决定角度。

典型流程：

```c
servo->pwm_driver = PWM_Create(gpio_port, gpio_pin);
PWM_OP(servo->pwm_driver, setConfig, 720 - 1, 2000 - 1); // 约 50Hz
PWM_OP(servo->pwm_driver, setDutyCycle, duty_cycle);      // 2.5%~12.5% 角度映射
```

---

## 5. 映射机制与引脚选择

驱动内部使用 `PWM_PinMap` 表按“从上到下第一个匹配”选择定时器与通道，支持：

- `TIM1 / TIM2 / TIM3 / TIM4`
- 高密度芯片下额外支持 `TIM5 / TIM8` 与部分高级重映射
- 互补通道`（CHxN）`与普通通道

### 5.1 引脚重复映射的优先级

部分引脚在不同模式下可对应多个通道，例如 `PA7`、`PB0`、`PB1`。  
由于驱动按映射表顺序匹配，最终会命中“更靠前的项”：

- `PA7` 会优先命中 `TIM1_CH1N`（部分重映射），而不是 `TIM3_CH2`
- `PB0` 会优先命中 `TIM1_CH2N`，而不是 `TIM3_CH3`
- `PB1` 会优先命中 `TIM1_CH3N`，而不是 `TIM3_CH4`


---

## 6. 使用注意点

1. **`PWM_Create` 自动 `start`**  
   创建后通道即进入输出状态（默认 0% 占空比），如果希望“先创建不输出”，需要先 `setDutyCycle(0)` 或调用 `stop`。

2. **占空比建议限定在 0~100**  
   `setDutyCycle` 当前未做显式边界裁剪，调用层应避免传入负值或超 100。

3. **互补通道占空比会反转**  
   对 TIM1/TIM8 的 CHxN，驱动内部会做 `actual = 100 - duty`，保证 **“设置 100% 表示全亮/全开”的一致语义**

4. **`setFrequency` 的实现策略是固定 ARR=1000**  
   频率通过调 PSC 计算，适合一般控制；若你更关注分辨率或精确波形，优先使用 `setConfig`。

5. **`stop` 只关当前通道，不关整个定时器**  
   这是为了兼容同一计时器多通道并行输出，不会误伤其他通道。

6. **资源管理**  
   驱动使用 `malloc/free`，建议在系统初始化阶段创建，在系统长期运行中避免频繁销毁重建。

---


## 7. 常见问题排查

1. **`PWM_Create` 返回 `NULL`**
   - 检查引脚是否在 `pwm_pin_map` 支持范围内。
   - 检查芯片密度宏（某些 TIM5/TIM8 映射只在 HD/XL/CL 编译条件下启用）。

2. **占空比设置了但波形没变化**
   - 检查外设是否需要反相（互补通道 CHxN 语义不同）。
   - 检查是否被其他模块反复覆盖占空比（如状态机循环里持续写入）。

3. **频率不符合预期**
   - 优先读取 `getPrescaler/getPeriod/getFrequency` 做运行时核对。
   - 舵机类应用建议改用 `setConfig` 明确 PSC/ARR。

4. **LED/FAN 行为反直觉**
   - 检查是否命中了重复映射引脚的“优先项”（特别是 `PA7/PB0/PB1`）。

---
