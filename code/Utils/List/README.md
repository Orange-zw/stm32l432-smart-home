# Embedded-List

一个专为嵌入式系统设计的C语言双向链表库，采用静态节点池和灵活数组设计，提供高效、安全、易用的链表操作接口。

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C Standard](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Platform](https://img.shields.io/badge/Platform-Embedded-green.svg)]()

## 📋 目录

- [特性](#特性)
- [为什么选择C语言实现](#为什么选择c语言实现)
- [为什么使用静态链表](#为什么使用静态链表)
- [嵌入式开发中的优势](#嵌入式开发中的优势)
- [快速开始](#快速开始)
- [API文档](#api文档)
- [使用示例](#使用示例)
- [线程安全](#线程安全)
- [内存管理](#内存管理)
- [数据持久化](#数据持久化)
- [性能考虑](#性能考虑)
- [局限性和缺点](#局限性和缺点)

## ✨ 特性

- 🎯 **嵌入式优化**：静态节点池预分配，避免运行时内存碎片
- 🔒 **线程安全**：可选的递归互斥锁支持（FreeRTOS/CMSIS-RTOS/Windows）
- 💾 **数据嵌入**：使用灵活数组将数据直接嵌入节点，减少内存分配次数
- 🔄 **双向链表**：支持高效的前向和后向遍历
- 🎨 **迭代器模式**：类似C++ STL的迭代器接口，使用指针索引
- 📦 **静态分配支持**：可从外部缓冲区创建，适合无动态内存的系统
- 🚀 **零依赖**：仅依赖标准C库，易于集成
- ✅ **完整测试**：包含全面的单元测试

## 🤔 为什么选择C语言实现

### 1. **嵌入式系统的标准语言**
C语言是嵌入式开发的主流语言，几乎所有微控制器都提供C编译器支持。使用C语言实现可以：
- 直接编译到目标平台，无需运行时环境
- 精确控制内存布局和分配
- 获得可预测的性能特征

### 2. **性能可控**
C语言提供了对底层硬件的直接访问能力：
- 无虚函数调用开销
- 无垃圾回收器干扰
- 编译后的代码体积小，执行效率高

### 3. **跨平台兼容性**
C语言标准（C99）被广泛支持，代码可以在不同架构间移植：
- ARM Cortex-M系列
- AVR系列
- RISC-V
- x86/x64（用于开发和测试）

### 4. **与现有代码集成**
大多数嵌入式项目使用C语言，本库可以无缝集成到现有代码库中。

## 🏗️ 为什么使用静态链表

### 传统动态链表的局限性

传统的动态链表每次插入都需要调用 `malloc()`，在嵌入式系统中会带来以下问题：

```c
// 传统动态链表的问题
void traditional_list_insert() {
    node_t *new_node = malloc(sizeof(node_t));  // ❌ 内存碎片
    new_node->data = malloc(data_size);          // ❌ 多次分配
    // ... 可能导致内存不足或碎片化
}
```

**问题：**
1. **内存碎片**：频繁的分配和释放导致堆内存碎片化
2. **分配失败风险**：长时间运行后可能出现分配失败
3. **实时性差**：`malloc()` 的执行时间不可预测
4. **内存泄漏风险**：需要手动管理每个节点的内存

### 静态链表的优势

本库采用**节点池（Node Pool）**设计，预先分配所有节点：

```c
// 静态链表：预先分配节点池
list_handle_t list = list_create(100, sizeof(int));  // 预分配100个节点
// 所有节点都在池中，插入/删除只是改变指针指向
```

**优势：**
1. ✅ **无内存碎片**：所有节点在连续内存中，不会产生碎片
2. ✅ **可预测性能**：插入/删除操作是O(1)，执行时间固定
3. ✅ **内存安全**：不会出现运行时分配失败
4. ✅ **适合实时系统**：操作时间可预测，满足实时性要求
5. ✅ **支持静态分配**：可以从外部缓冲区创建，无需动态内存

### 节点池工作原理

```
节点池布局：
[node_pool]
  ↓
[node0+data] → [node1+data] → [node2+data] → ... → [nodeN+data]
  ↑              ↑              ↑                      ↑
free_list      空闲节点        空闲节点              空闲节点

使用中的链表：
head → [node5+data] → [node2+data] → [node8+data] → NULL
        ↑              ↑              ↑
      已使用         已使用          已使用
```

## 🎯 嵌入式开发中的优势

### 1. **内存受限环境友好**

```c
// 在RAM只有几KB的MCU上使用
#define MAX_ITEMS 50
#define ELEMENT_SIZE sizeof(int)
// 注意：需要分配足够的缓冲区，考虑对齐后的节点大小
static uint8_t node_pool[(sizeof(list_node_t) + ELEMENT_SIZE + 4) * MAX_ITEMS];
list_handle_t list = list_create_from_buf(node_pool, MAX_ITEMS, sizeof(int));
// ✅ 内存使用完全可控，不会超出预期
// ✅ 静态数组会自动4字节对齐，满足ARM架构要求
```

### 2. **实时性保证**

```c
// 在中断服务程序中使用
void ISR_Handler() {
    int data = read_sensor();
    list_push_back(sensor_list, &data);  // ✅ O(1)操作，时间可预测
}
```

### 3. **无动态内存分配**

```c
// 适合禁用malloc的严格嵌入式环境
// 使用静态缓冲区
static uint8_t buffer[1024];
list_handle_t list = list_create_from_buf(buffer, 100, sizeof(int));
// ✅ 完全不依赖堆内存
```

### 4. **低功耗考虑**

- 预分配避免了频繁的内存分配操作
- 减少了CPU唤醒次数
- 内存访问模式更规律，有利于缓存

### 5. **代码体积小**

- 无复杂的动态内存管理代码
- 编译后的二进制体积小
- 适合Flash空间受限的MCU

## 🚀 快速开始

```
# 项目结构
Algorithm/List/
├── embedded_list.h     # 主头文件
├── embedded_list.c     # 核心实现
├── list_save.h         # 数据持久化头文件
├── list_save.c         # 数据持久化实现
│
├── test_list.h         # 单元测试头文件
├── test_list.c         # 单元测试实现
└── test_main.c         # 测试主程序
```

### 1. 包含头文件

```c
#include "embedded_list.h"
```

### 2. 创建链表

```c
// 动态分配方式
list_handle_t list = list_create(100, sizeof(int));  // 容量100，元素类型int

// 静态分配方式（适合无动态内存的系统）
static uint8_t buffer[1024];
list_handle_t list = list_create_from_buf(buffer, 100, sizeof(int));
```

### 3. 基本操作

```c
// 添加元素
int value = 42;
list_push_back(list, &value);

// 遍历
list_iterator_t it = list_begin(list);
while (it != NULL) {
    int *data = (int *)it->data;
    printf("%d\n", *data);
    it = list_next(it);
}

// 删除元素
list_iterator_t to_remove = list_find(list, &value);
if (to_remove != NULL) {
    list_erase(list, to_remove);
}

// 清理
list_free(list);
```

## 📚 API文档

### 创建和销毁

| 函数 | 说明 |
|------|------|
| `list_create(capacity, element_size)` | 动态创建链表 |
| `list_create_from_buf(buf, capacity, element_size)` | 从缓冲区创建链表 |
| `list_free(list)` | 释放链表 |

### 容量查询

| 函数 | 说明 |
|------|------|
| `list_empty(list)` | 检查是否为空 |
| `list_size(list)` | 获取元素数量 |
| `list_capacity(list)` | 获取最大容量 |

### 元素访问

| 函数 | 说明 |
|------|------|
| `list_front(list, element)` | 获取首元素 |
| `list_back(list, element)` | 获取尾元素 |
| `list_begin(list)` | 获取首迭代器 |
| `list_end(list)` | 获取尾迭代器 |
| `list_next(it)` | 下一个迭代器 |
| `list_prev(it)` | 上一个迭代器 |
| `list_at(list, index)` | 通过索引获取迭代器 |
| `list_get(list, index)` | 通过索引获取数据指针 |

### 修改操作

| 函数 | 说明 |
|------|------|
| `list_push_front(list, element)` | 头部插入 |
| `list_push_back(list, element)` | 尾部插入 |
| `list_pop_front(list, element)` | 头部删除 |
| `list_pop_back(list, element)` | 尾部删除 |
| `list_insert(list, position, element)` | 指定位置插入 |
| `list_erase(list, position)` | 删除指定位置 |
| `list_replace(list, position, element)` | 替换元素 |
| `list_clear(list)` | 清空链表 |

### 列表操作

| 函数 | 说明 |
|------|------|
| `list_splice(list1, pos, list2, first, last)` | 拼接操作 |
| `list_merge(list1, list2)` | 合并两个链表 |
| `list_remove(list, value)` | 删除所有匹配值 |
| `list_remove_if(list, predicate, data)` | 条件删除 |
| `list_reverse(list)` | 反转链表 |
| `list_unique(list)` | 去重 |

### 查找操作

| 函数 | 说明 |
|------|------|
| `list_find(list, value)` | 查找值 |
| `list_find_if(list, start, predicate, data)` | 条件查找 |
| `list_contains(list, value)` | 检查是否包含 |

### 数据持久化（list_save.h）

| 函数 | 说明 |
|------|------|
| `list_serialize(list, buffer, buffer_size)` | 序列化链表到缓冲区 |
| `list_deserialize(list, buffer, buffer_size)` | 从缓冲区反序列化链表 |
| `list_get_serialize_size(list)` | 计算序列化所需缓冲区大小 |

## 💡 使用示例

### 示例1：传感器数据采集

```c
#include "embedded_list.h"

// 传感器数据结构
typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_data_t;

void sensor_collection_example() {
    // 创建链表，存储最近100个传感器读数
    list_handle_t sensor_list = list_create(100, sizeof(sensor_data_t));
    
    // 采集数据
    sensor_data_t data = {
        .temperature = 25.5f,
        .humidity = 60.0f,
        .timestamp = get_timestamp()
    };
    list_push_back(sensor_list, &data);
    
    // 遍历所有数据
    list_iterator_t it = list_begin(sensor_list);
    while (it != NULL) {
        sensor_data_t *s = (sensor_data_t *)it->data;
        printf("Temp: %.1f, Humidity: %.1f\n", s->temperature, s->humidity);
        it = list_next(it);
    }
    
    // 当链表满时，删除最旧的数据
    if (list_size(sensor_list) >= list_capacity(sensor_list)) {
        list_pop_front(sensor_list, NULL);
    }
    
    list_free(sensor_list);
}
```

### 示例2：事件队列

```c
#include "embedded_list.h"

typedef enum {
    EVENT_BUTTON_PRESS,
    EVENT_SENSOR_TRIGGER,
    EVENT_TIMER_EXPIRE
} event_type_t;

typedef struct {
    event_type_t type;
    uint32_t param;
} event_t;

void event_queue_example() {
    list_handle_t event_queue = list_create(50, sizeof(event_t));
    
    // 添加事件
    event_t evt = {EVENT_BUTTON_PRESS, 1};
    list_push_back(event_queue, &evt);
    
    // 处理事件队列
    while (!list_empty(event_queue)) {
        event_t evt;
        list_pop_front(event_queue, &evt);
        
        switch (evt.type) {
            case EVENT_BUTTON_PRESS:
                handle_button(evt.param);
                break;
            // ...
        }
    }
    
    list_free(event_queue);
}
```

### 示例3：条件查找和删除

```c
#include "embedded_list.h"

// 查找所有大于阈值的值
bool is_greater_than(const void *list_data, const void *predicate_data) {
    int value = *(const int *)list_data;
    int threshold = *(const int *)predicate_data;
    return value > threshold;
}

void conditional_operations_example() {
    list_handle_t list = list_create(100, sizeof(int));
    
    // 添加一些数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        list_push_back(list, &values[i]);
    }
    
    // 查找所有大于25的值
    int threshold = 25;
    list_iterator_t it = list_find_if(list, NULL, is_greater_than, &threshold);
    while (it != NULL) {
        printf("Found: %d\n", *(int *)it->data);
        it = list_find_if(list, it, is_greater_than, &threshold);
    }
    
    // 删除所有大于25的值
    uint16_t removed = list_remove_if(list, is_greater_than, &threshold);
    printf("Removed %d elements\n", removed);
    
    list_free(list);
}
```

### 示例4：静态分配（无动态内存）

```c
#include "embedded_list.h"

void static_allocation_example() {
    // 在栈上分配节点池
    // 注意：需要分配足够的缓冲区，考虑对齐后的节点大小（每个节点会自动对齐到4字节）
    #define MAX_ITEMS 50
    #define ELEMENT_SIZE sizeof(int)
    static uint8_t node_pool[(sizeof(list_node_t) + ELEMENT_SIZE + 4) * MAX_ITEMS];
    
    // 从缓冲区创建链表（静态数组会自动4字节对齐，满足ARM架构要求）
    list_handle_t list = list_create_from_buf(node_pool, MAX_ITEMS, sizeof(int));
    if (list == NULL) {
        // 创建失败，可能是缓冲区未对齐（静态数组通常不会有此问题）
        return;
    }
    
    // 正常使用
    int value = 42;
    list_push_back(list, &value);
    
    // 不需要调用list_free，因为使用的是外部缓冲区
    // 但可以调用list_clear清空数据
    list_clear(list);
}
```

## 🔒 线程安全

支持可选的线程安全功能，使用递归互斥锁：

### 启用线程安全

```c
// 在包含头文件前定义
#define LIST_ENABLE_THREAD_SAFE
#include "embedded_list.h"
```

### 支持的平台

- **FreeRTOS**: 自动使用 `xSemaphoreCreateRecursiveMutex()`
- **CMSIS-RTOS**: 自动使用 `osMutexNew()`
- **Windows**: 自动使用 `CreateMutex()`
- **自定义锁**: 通过 `LIST_CUSTOM_LOCK` 定义

### 递归锁的优势

递归锁允许同一线程多次获取锁，避免了死锁问题：

```c
// 安全：list_remove_if内部会调用list_erase，两者都需要锁
uint16_t count = list_remove_if(list, predicate, data);  // ✅ 不会死锁
```

## 💾 内存管理

### 动态分配模式

```c
list_handle_t list = list_create(100, sizeof(int));
// 分配了：
// - list_t结构体
// - 100个节点的节点池（每个节点包含int数据）
// 使用完毕后调用：
list_free(list);  // 释放所有内存
```

### 静态分配模式

```c
static uint8_t buffer[1024];
list_handle_t list = list_create_from_buf(buffer, 100, sizeof(int));
// 只分配了list_t结构体，节点池使用外部缓冲区
// 不需要调用list_free（但可以调用list_clear清空数据）
```

### 内存布局

```
节点结构：
[list_node_t] [data...]
  ↑            ↑
指针部分    数据部分（灵活数组）

节点池布局：
[node0+data] [node1+data] [node2+data] ... [nodeN+data]
  ↑            ↑            ↑                 ↑
连续内存块，无碎片
```

### ⚠️ 内存对齐要求（重要）

**在ARM架构（如STM32F103、Cortex-M系列）上，必须注意内存对齐问题：**

#### 1. 节点大小自动对齐

本库已经自动处理了节点大小的对齐问题。每个节点的大小会自动向上对齐到4字节边界，确保在ARM架构上正确访问指针成员，避免Hard Fault错误。

**对齐规则：**
- 库内部会自动将节点大小对齐到4字节边界
- 节点大小 = 向上对齐到4字节的 `(sizeof(list_node_t) + element_size)`
- 例如：`element_size = 1` 时，节点大小 = `对齐(8 + 1, 4) = 12` 字节
- `sizeof(list_node_t)` 通常是 8 字节（包含两个指针：next 和 prev）

#### 2. 缓冲区地址对齐要求

使用 `list_create_from_buf()` 时，**传入的缓冲区地址必须是4字节对齐的**：

```c
// ✅ 正确：静态数组会自动对齐到4字节边界
static uint8_t buffer[1024];
list_handle_t list = list_create_from_buf(buffer, 100, sizeof(int));

// ✅ 正确：使用对齐属性（GCC/ARMCC）
__attribute__((aligned(4))) uint8_t buffer[1024];
list_handle_t list = list_create_from_buf(buffer, 100, sizeof(int));

// ❌ 错误：缓冲区未对齐会导致Hard Fault（在ARM架构上）
uint8_t *buffer = malloc(1024) + 1;  // 偏移1字节，未对齐
list_handle_t list = list_create_from_buf(buffer, 100, sizeof(int));  // 会返回NULL或触发Hard Fault
```

**检查对齐的方法：**
```c
// 确保缓冲区4字节对齐
if (((unsigned long)buffer & 3) != 0) {
    // 缓冲区未对齐，需要调整
}
```

#### 3. 为什么需要对齐？

- **ARM架构要求**：32位ARM处理器要求指针访问必须4字节对齐
- **性能优化**：对齐的内存访问更快
- **避免Hard Fault**：未对齐的指针访问会导致硬件异常（Hard Fault）

#### 4. 动态分配模式

使用 `list_create()` 时，`malloc()` 返回的地址通常会自动对齐到8字节或更大边界，所以不需要担心对齐问题。

#### 5. 实际大小计算

由于对齐的存在，实际节点池大小可能比理论值稍大：

```c
// 理论大小（不考虑对齐）
size_t theoretical_size = sizeof(list_node_t) * capacity + capacity * element_size;
// sizeof(list_node_t) = 8 字节（两个指针：next 和 prev）

// 实际大小（库内部会自动对齐到4字节边界）
// 每个节点大小 = 向上对齐到4字节的 (8 + element_size)
// 例如：capacity=100, element_size=1 时
// 理论：8*100 + 1*100 = 900 字节
// 实际：12*100 = 1200 字节（每个节点对齐到12字节，即 ALIGN_UP(9, 4) = 12）

// 对于小元素（element_size < 4），每个节点通常需要 12 字节
// 对于大元素（element_size >= 4 且是4的倍数），每个节点 = 8 + element_size
// 对于大元素（element_size >= 4 但不是4的倍数），每个节点 = 8 + element_size + 对齐填充
```

**建议：** 在使用静态分配时，建议分配足够大的缓冲区。由于库内部会自动对齐节点大小，可以按以下方式计算：

```c
#define MAX_ITEMS 100
#define ELEMENT_SIZE sizeof(int)
// 保守估计：sizeof(list_node_t)通常是8字节（两个指针），加上数据大小，然后向上对齐到4字节
// 对于小数据（<4字节），每个节点大约12-16字节
// 对于大数据，每个节点 = sizeof(list_node_t) + ELEMENT_SIZE + 对齐填充
#define ESTIMATED_NODE_SIZE (sizeof(list_node_t) + ELEMENT_SIZE + 4)  // 保守估计，多加4字节对齐余量
static uint8_t node_pool[ESTIMATED_NODE_SIZE * MAX_ITEMS];

// 或者使用更简单的方法：分配稍大的缓冲区（推荐）
// 对于小元素（如int），每个节点大约12字节；对于大元素，节点大小接近 element_size + 8
static uint8_t node_pool[(sizeof(list_node_t) + ELEMENT_SIZE + 4) * MAX_ITEMS];
```

**注意：** 如果缓冲区分配不够大，`list_create_from_buf()` 仍会成功创建链表，但后续操作可能会覆盖缓冲区边界，导致未定义行为。建议分配足够的缓冲区大小。

## 💾 数据持久化

提供了 `list_save.h` 和 `list_save.c` 文件，支持将链表数据序列化到缓冲区，以便保存到`Flash`、`EEPROM`或其他非易失性存储设备。

### 功能说明

**数据持久化模块**允许你将链表的状态保存到缓冲区，并在系统重启后恢复。这对于需要保存配置数据、历史记录或状态信息的嵌入式应用非常有用。

### 核心功能

1. **序列化（Serialize）**：将链表数据转换为连续的字节流
2. **反序列化（Deserialize）**：从字节流恢复链表数据
3. **大小计算**：预先计算序列化所需缓冲区大小

### 数据格式

序列化后的数据格式：

```
[list_persist_header_t]
  - size: 元素数量
  - capacity: 容量
  - element_size: 元素大小
[nodes数组]
  - [index0][data0]
  - [index1][data1]
  - ...
  - [indexN][dataN]
```

每个节点保存其在节点池中的索引和实际数据，这样可以正确恢复链表的逻辑顺序。

### 使用示例

#### 保存到Flash

```c
#include "embedded_list.h"
#include "list_save.h"

void save_to_flash_example() {
    list_handle_t list = list_create(100, sizeof(int));
    
    // 添加一些数据
    for (int i = 0; i < 10; i++) {
        list_push_back(list, &i);
    }
    
    // 计算所需缓冲区大小
    uint32_t size = list_get_serialize_size(list);
    
    // 分配缓冲区（可以从Flash或外部存储分配）
    uint8_t *buffer = malloc(size);
    
    // 序列化
    uint32_t written = list_serialize(list, buffer, size);
    if (written > 0) {
        // 保存到Flash
        flash_write(FLASH_ADDR, buffer, written);
        printf("Saved %u bytes to flash\n", written);
    }
    
    free(buffer);
    list_free(list);
}
```

#### 从Flash恢复

```c
#include "embedded_list.h"
#include "list_save.h"

void restore_from_flash_example() {
    // 创建链表（容量和元素大小必须与保存时一致）
    list_handle_t list = list_create(100, sizeof(int));
    
    // 从Flash读取
    uint8_t buffer[1024];
    uint32_t size = flash_read(FLASH_ADDR, buffer, sizeof(buffer));
    
    // 反序列化
    if (list_deserialize(list, buffer, size)) {
        printf("Restored %u elements\n", list_size(list));
        
        // 验证数据
        list_iterator_t it = list_begin(list);
        while (it != NULL) {
            printf("%d ", *(int *)it->data);
            it = list_next(it);
        }
        printf("\n");
    } else {
        printf("Deserialize failed\n");
    }
    
    list_free(list);
}
```

#### 断电保护场景

```c
#include "embedded_list.h"
#include "list_save.h"

// 传感器数据记录，需要断电保护
void sensor_log_with_persistence() {
    #define MAX_READINGS 50
    static uint8_t node_pool[sizeof(list_node_t) * MAX_READINGS + 
                              MAX_READINGS * sizeof(sensor_data_t)];
    
    list_handle_t readings = list_create_from_buf(node_pool, MAX_READINGS, 
                                             sizeof(sensor_data_t));
    
    // 定期保存到EEPROM
    void periodic_save() {
        uint32_t size = list_get_serialize_size(readings);
        static uint8_t save_buffer[512];
        
        if (size <= sizeof(save_buffer)) {
            uint32_t written = list_serialize(readings, save_buffer, size);
            if (written > 0) {
                eeprom_write(EEPROM_ADDR, save_buffer, written);
            }
        }
    }
    
    // 系统启动时恢复
    void restore_on_startup() {
        uint8_t buffer[512];
        uint32_t size = eeprom_read(EEPROM_ADDR, buffer, sizeof(buffer));
        list_deserialize(readings, buffer, size);
    }
}
```

### ⚠️注意事项

1. **容量要求**：反序列化时，新链表的 `capacity` 必须 **大于等于** 旧链表的 `capacity`
2. **元素大小必须一致**：`element_size` 必须与序列化时完全一致
3. **缓冲区大小**：确保缓冲区足够大，可以使用 `list_get_serialize_size()` 预先计算
4. **数据完整性**：序列化数据包含校验信息，反序列化时会验证数据有效性
5. **线程安全**：序列化和反序列化操作都是线程安全的
6. **内存分配**：反序列化过程中会临时分配少量内存（用于标记已使用的节点）



### 序列化大小计算

序列化后的数据大小：

```
总大小 = sizeof(list_persist_header_t) + size × (sizeof(uint16_t) + element_size)
```

其中：
- `list_persist_header_t`: 12字节（size + capacity + element_size）
- 每个节点：2字节（索引） + element_size（数据）

### 适用场景

- ✅ 配置数据保存（系统参数、用户设置）
- ✅ 历史数据记录（传感器读数、日志）
- ✅ 状态保存（游戏进度、工作状态）
- ✅ 断电保护（关键数据持久化）
- ❌ 不适合：频繁读写（Flash有擦写次数限制）
- ❌ 不适合：大数据量（受Flash容量限制）

## ⚡ 性能考虑

### 时间复杂度

| 操作 | 时间复杂度 | 说明 |
|------|-----------|------|
| `push_front/back` | O(1) | 常数时间 |
| `pop_front/back` | O(1) | 常数时间 |
| `insert/erase` | O(1) | 给定迭代器位置 |
| `find` | O(n) | 需要遍历 |
| `at/get` | O(n) | 需要遍历到指定位置 |
| `reverse` | O(n) | 需要遍历所有节点 |
| `unique` | O(n²) | 嵌套循环 |

### 空间复杂度

- **节点池**: O(capacity × (sizeof(node) + element_size))
- **链表结构**: O(1)
- **总空间**: 可预测，无额外开销

### 建议

1. **合理设置容量**：根据实际需求设置，避免浪费
2. **使用迭代器**：避免使用 `list_at()` 进行随机访问
3. **批量操作**：使用 `list_splice()` 进行批量移动
4. **静态分配**：在内存受限环境中使用静态分配模式

## ⚠️ 局限性和缺点

### 1. **固定容量限制**

**问题：**
- 链表容量在创建时固定，无法动态扩展
- 如果容量设置过小，可能导致插入失败

**影响：**
```c
list_handle_t list = list_create(10, sizeof(int));  // 容量固定为10
// 如果尝试插入第11个元素，会失败
if (!list_push_back(list, &value)) {
    // 容量已满，插入失败
}
```

**解决方案：**

- 根据实际需求合理设置容量
- 在插入前检查 `list_size() < list_capacity()`
- 如果容量不足，删除旧数据或使用更大的容量

### 2. **内存预分配**

**问题：**
- 即使链表为空，也会占用完整的节点池内存
- 如果容量设置过大，会浪费内存

**影响：**
```c
// 即使只存储1个元素，也会分配100个节点的内存
list_handle_t list = list_create(100, sizeof(int));
list_push_back(list, &value);  // 只用了1个节点，但占用了100个节点的内存
```

**解决方案：**
- 根据实际最大需求设置容量
- 使用 `list_create_from_buf()` 从外部缓冲区分配，更好地控制内存来源

### 3. **随机访问性能差**

**问题：**
- 通过索引访问元素需要O(n)时间复杂度
- 不适合需要频繁随机访问的场景

**影响：**
```c
// 访问第50个元素需要遍历50个节点
void *data = list_get(list, 50);  // O(n)操作
```

**解决方案：**
- 使用迭代器进行顺序访问（O(1)）
- 如果需要频繁随机访问，考虑使用数组或动态数组

### 4. **查找操作较慢**

**问题：**
- `list_find()` 和 `list_find_if()` 需要遍历链表
- 时间复杂度为O(n)

**影响：**
```c
// 在1000个元素中查找需要最多1000次比较
list_iterator_t it = list_find(list, &target);
```

**解决方案：**
- 如果频繁查找，考虑使用哈希表或有序数组+二分查找
- 对于小规模数据（<100元素），性能影响可接受

### 5. **去重操作效率低**

**问题：**
- `list_unique()` 使用嵌套循环，时间复杂度O(n²)
- 对于大量数据性能较差

**影响：**
```c
// 1000个元素需要约100万次比较
uint16_t removed = list_unique(list);
```

**解决方案：**
- 对于大数据量，考虑先排序再去重（需要额外实现）
- 或者使用其他数据结构（如集合）

### 6. **不支持动态扩容**

**问题：**
- 无法像动态数组那样自动扩容
- 容量不足时需要手动处理

**影响：**
```c
// 无法自动扩展容量
if (list_size(list) >= list_capacity(list)) {
    // 需要手动处理：删除旧数据或创建新链表
}
```

**解决方案：**
- 这是设计选择，为了可预测的内存使用
- 在创建时设置足够的容量
- 实现自己的容量管理逻辑

### 7. **序列化需要额外内存**

**问题：**
- `list_deserialize()` 会临时分配内存（用于标记已使用的节点）
- 在内存极度受限的环境中可能失败

**影响：**
```c
// 反序列化时会分配 capacity × sizeof(bool) 的临时内存
bool success = list_deserialize(list, buffer, size);
```

**解决方案：**
- 确保系统有足够的堆内存
- 对于极度受限的环境，可以考虑优化实现（使用位图代替bool数组）

### 8. **元素大小固定**

**问题：**
- 每个链表只能存储固定大小的元素
- 无法存储不同大小的元素

**影响：**
```c
// 无法在同一链表中存储不同大小的结构
list_handle_t list = list_create(100, sizeof(int));  // 只能存储int
// 无法存储不同大小的结构体
```

**解决方案：**
- 使用联合体（union）或固定大小的结构
- 存储指向数据的指针（需要额外管理数据内存）



### 以下场景可能不适合使用本库

1. **需要动态扩容**：使用动态数组（如 `realloc()` 实现的数组）
2. **频繁随机访问**：使用数组或动态数组
3. **需要快速查找**：使用哈希表或有序数组
4. **存储变长数据**：使用其他数据结构或存储指针
5. **大数据量去重**：使用集合（Set）数据结构
6. **需要自动排序**：使用有序数组或平衡树

### 总结

本库的设计目标是：
- ✅ **可预测的内存使用**（固定容量）
- ✅ **实时性保证**（O(1)插入/删除）
- ✅ **嵌入式友好**（无内存碎片）
- ✅ **简单易用**（类似STL的接口）

**这些优点是以牺牲某些灵活性为代价的。在选择使用本库时，请确保你的应用场景与这些设计目标匹配。**

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

详细的贡献指南请参阅 [CONTRIBUTING.md](CONTRIBUTING.md)。更多使用示例参考 [EXAMPLES.md](EXAMPLES.md)

