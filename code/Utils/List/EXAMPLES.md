# Embedded-List 使用示例

本文档提供了更多实际使用场景的示例代码。

## 目录

- [基础示例](#基础示例)
- [高级示例](#高级示例)
- [嵌入式场景](#嵌入式场景)

## 基础示例

### 示例1：整数列表

```c
#include "embedded_list.h"
#include <stdio.h>

void basic_int_list() {
    // 创建容量为10的整数列表
    list_t *list = list_create(10, sizeof(int));
    
    // 添加元素
    for (int i = 0; i < 5; i++) {
        list_push_back(list, &i);
    }
    
    // 遍历打印
    printf("List contents: ");
    list_iterator_t it = list_begin(list);
    while (it != NULL) {
        printf("%d ", *(int *)it->data);
        it = list_next(it);
    }
    printf("\n");
    
    // 查找元素
    int target = 3;
    list_iterator_t found = list_find(list, &target);
    if (found != NULL) {
        printf("Found: %d\n", *(int *)found->data);
    }
    
    list_free(list);
}
```

### 示例2：结构体列表

```c
#include "embedded_list.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int id;
    char name[32];
    float score;
} student_t;

void struct_list_example() {
    list_t *students = list_create(50, sizeof(student_t));
    
    // 添加学生
    student_t s1 = {1, "Alice", 95.5f};
    student_t s2 = {2, "Bob", 87.0f};
    student_t s3 = {3, "Charlie", 92.5f};
    
    list_push_back(students, &s1);
    list_push_back(students, &s2);
    list_push_back(students, &s3);
    
    // 查找高分学生（>90分）
    bool is_high_score(const void *data, const void *predicate_data) {
        const student_t *s = (const student_t *)data;
        float threshold = *(const float *)predicate_data;
        return s->score > threshold;
    }
    
    float threshold = 90.0f;
    list_iterator_t it = list_find_if(students, NULL, is_high_score, &threshold);
    while (it != NULL) {
        student_t *s = (student_t *)it->data;
        printf("High score: %s (%.1f)\n", s->name, s->score);
        it = list_find_if(students, it, is_high_score, &threshold);
    }
    
    list_free(students);
}
```

## 高级示例

### 示例3：实现队列（FIFO）

```c
#include "embedded_list.h"

typedef struct {
    list_t *list;
} queue_t;

queue_t *queue_create(uint16_t capacity, uint16_t element_size) {
    queue_t *q = malloc(sizeof(queue_t));
    q->list = list_create(capacity, element_size);
    return q;
}

void queue_enqueue(queue_t *q, const void *element) {
    list_push_back(q->list, element);
}

bool queue_dequeue(queue_t *q, void *element) {
    return list_pop_front(q->list, element);
}

bool queue_empty(queue_t *q) {
    return list_empty(q->list);
}

uint16_t queue_size(queue_t *q) {
    return list_size(q->list);
}

void queue_free(queue_t *q) {
    list_free(q->list);
    free(q);
}
```

### 示例4：实现栈（LIFO）

```c
#include "embedded_list.h"

typedef struct {
    list_t *list;
} stack_t;

stack_t *stack_create(uint16_t capacity, uint16_t element_size) {
    stack_t *s = malloc(sizeof(stack_t));
    s->list = list_create(capacity, element_size);
    return s;
}

void stack_push(stack_t *s, const void *element) {
    list_push_front(s->list, element);
}

bool stack_pop(stack_t *s, void *element) {
    return list_pop_front(s->list, element);
}

bool stack_empty(stack_t *s) {
    return list_empty(s->list);
}

uint16_t stack_size(stack_t *s) {
    return list_size(s->list);
}

void stack_free(stack_t *s) {
    list_free(s->list);
    free(s);
}
```

### 示例5：LRU缓存实现

```c
#include "embedded_list.h"

typedef struct {
    int key;
    int value;
} cache_entry_t;

typedef struct {
    list_t *list;
    uint16_t max_size;
} lru_cache_t;

lru_cache_t *lru_create(uint16_t max_size) {
    lru_cache_t *cache = malloc(sizeof(lru_cache_t));
    cache->list = list_create(max_size, sizeof(cache_entry_t));
    cache->max_size = max_size;
    return cache;
}

void lru_put(lru_cache_t *cache, int key, int value) {
    // 查找是否已存在
    cache_entry_t target = {key, 0};
    list_iterator_t it = list_find(cache->list, &target);
    
    if (it != NULL) {
        // 更新值并移到头部
        cache_entry_t *entry = (cache_entry_t *)it->data;
        entry->value = value;
        list_erase(cache->list, it);
        list_push_front(cache->list, entry);
    } else {
        // 添加新条目
        cache_entry_t entry = {key, value};
        
        // 如果缓存已满，删除最旧的（尾部）
        if (list_size(cache->list) >= cache->max_size) {
            list_pop_back(cache->list, NULL);
        }
        
        list_push_front(cache->list, &entry);
    }
}

int lru_get(lru_cache_t *cache, int key) {
    cache_entry_t target = {key, 0};
    list_iterator_t it = list_find(cache->list, &target);
    
    if (it != NULL) {
        cache_entry_t *entry = (cache_entry_t *)it->data;
        // 移到头部（标记为最近使用）
        list_erase(cache->list, it);
        list_push_front(cache->list, entry);
        return entry->value;
    }
    
    return -1;  // 未找到
}
```

## 嵌入式场景

### 示例6：传感器数据缓冲区

```c
#include "embedded_list.h"

typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_reading_t;

void sensor_buffer_example() {
    // 使用静态分配，避免动态内存
    #define MAX_READINGS 100
    static uint8_t buffer[sizeof(list_node_t) * MAX_READINGS + 
                          MAX_READINGS * sizeof(sensor_reading_t)];
    
    list_t *readings = list_create_from_buf(buffer, MAX_READINGS, 
                                             sizeof(sensor_reading_t));
    
    // 采集传感器数据
    sensor_reading_t reading = {
        .temperature = read_temperature(),
        .humidity = read_humidity(),
        .timestamp = get_timestamp()
    };
    
    list_push_back(readings, &reading);
    
    // 当缓冲区满时，删除最旧的数据
    if (list_size(readings) >= list_capacity(readings)) {
        list_pop_front(readings, NULL);
    }
    
    // 计算平均温度
    float sum = 0.0f;
    uint16_t count = 0;
    list_iterator_t it = list_begin(readings);
    while (it != NULL) {
        sensor_reading_t *r = (sensor_reading_t *)it->data;
        sum += r->temperature;
        count++;
        it = list_next(it);
    }
    
    if (count > 0) {
        float avg_temp = sum / count;
        printf("Average temperature: %.2f\n", avg_temp);
    }
}
```

### 示例7：命令队列（中断安全）

```c
#include "embedded_list.h"

typedef enum {
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_BUZZER_BEEP,
    CMD_DISPLAY_UPDATE
} command_type_t;

typedef struct {
    command_type_t type;
    uint32_t param;
} command_t;

// 全局命令队列
static list_t *cmd_queue = NULL;

void command_queue_init() {
    // 使用静态分配
    static uint8_t buffer[sizeof(list_node_t) * 50 + 50 * sizeof(command_t)];
    cmd_queue = list_create_from_buf(buffer, 50, sizeof(command_t));
}

// 在中断中调用（添加命令）
void ISR_AddCommand(command_type_t type, uint32_t param) {
    command_t cmd = {type, param};
    list_push_back(cmd_queue, &cmd);
}

// 在主循环中调用（处理命令）
void process_commands() {
    while (!list_empty(cmd_queue)) {
        command_t cmd;
        list_pop_front(cmd_queue, &cmd);
        
        switch (cmd.type) {
            case CMD_LED_ON:
                led_on();
                break;
            case CMD_LED_OFF:
                led_off();
                break;
            case CMD_BUZZER_BEEP:
                buzzer_beep(cmd.param);
                break;
            case CMD_DISPLAY_UPDATE:
                display_update(cmd.param);
                break;
        }
    }
}
```

### 示例8：任务调度器

```c
#include "embedded_list.h"

typedef struct {
    void (*task_func)(void);
    uint32_t period_ms;
    uint32_t last_run;
    bool enabled;
} task_t;

typedef struct {
    list_t *tasks;
    uint32_t tick_count;
} scheduler_t;

scheduler_t *scheduler_create(uint16_t max_tasks) {
    scheduler_t *sched = malloc(sizeof(scheduler_t));
    sched->tasks = list_create(max_tasks, sizeof(task_t));
    sched->tick_count = 0;
    return sched;
}

void scheduler_add_task(scheduler_t *sched, void (*func)(void), 
                        uint32_t period_ms) {
    task_t task = {
        .task_func = func,
        .period_ms = period_ms,
        .last_run = 0,
        .enabled = true
    };
    list_push_back(sched->tasks, &task);
}

void scheduler_tick(scheduler_t *sched) {
    sched->tick_count++;
    
    list_iterator_t it = list_begin(sched->tasks);
    while (it != NULL) {
        task_t *task = (task_t *)it->data;
        
        if (task->enabled && 
            (sched->tick_count - task->last_run) >= task->period_ms) {
            task->task_func();
            task->last_run = sched->tick_count;
        }
        
        it = list_next(it);
    }
}
```

### 示例9：数据日志记录

```c
#include "embedded_list.h"

typedef struct {
    uint8_t level;  // 日志级别
    char message[64];
    uint32_t timestamp;
} log_entry_t;

void logging_example() {
    list_t *log_buffer = list_create(200, sizeof(log_entry_t));
    
    // 添加日志
    log_entry_t entry = {
        .level = 1,
        .message = "System started",
        .timestamp = get_timestamp()
    };
    list_push_back(log_buffer, &entry);
    
    // 查找错误级别的日志
    bool is_error(const void *data, const void *predicate_data) {
        const log_entry_t *entry = (const log_entry_t *)data;
        return entry->level >= 3;  // 错误级别 >= 3
    }
    
    list_iterator_t it = list_find_if(log_buffer, NULL, is_error, NULL);
    while (it != NULL) {
        log_entry_t *entry = (log_entry_t *)it->data;
        printf("ERROR: %s\n", entry->message);
        it = list_find_if(log_buffer, it, is_error, NULL);
    }
    
    // 当缓冲区满时，删除最旧的日志
    if (list_size(log_buffer) >= list_capacity(log_buffer)) {
        list_pop_front(log_buffer, NULL);
    }
    
    list_free(log_buffer);
}
```

## 性能优化技巧

### 1. 使用静态分配避免堆碎片

```c
// 推荐：静态分配
static uint8_t buffer[1024];
list_t *list = list_create_from_buf(buffer, 100, sizeof(int));

// 不推荐：动态分配（在长时间运行的系统中可能产生碎片）
list_t *list = list_create(100, sizeof(int));
```

### 2. 合理设置容量

```c
// 根据实际需求设置，不要过大
#define MAX_ITEMS 50  // 实际最多需要50个
list_t *list = list_create(MAX_ITEMS, sizeof(int));
```

### 3. 使用迭代器而不是索引访问

```c
// 推荐：O(1)访问
list_iterator_t it = list_begin(list);
while (it != NULL) {
    process(it->data);
    it = list_next(it);
}

// 不推荐：O(n)访问
for (int i = 0; i < list_size(list); i++) {
    void *data = list_get(list, i);  // 需要遍历
    process(data);
}
```

### 4. 批量操作使用splice

```c
// 高效：批量移动节点
list_splice(list1, NULL, list2, first, last);

// 低效：逐个移动
while (first != last) {
    void *data = first->data;
    list_push_back(list1, data);
    list_erase(list2, first);
    first = list_next(first);
}
```

---

更多示例和最佳实践，请参考 [README.md](README.md)。

