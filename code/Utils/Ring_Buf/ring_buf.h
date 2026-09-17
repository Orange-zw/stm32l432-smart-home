#ifndef RING_BUF_H
#define RING_BUF_H

#include <stdbool.h>
#include <stdint.h>

// ========================= 线程安全配置宏 =========================
// 默认启用线程安全，如果不需要线程安全，可以在包含本头文件前定义 RINGBUF_DISABLE_THREAD_SAFE
#define RINGBUF_DISABLE_THREAD_SAFE
#ifndef RINGBUF_DISABLE_THREAD_SAFE
#define RINGBUF_THREAD_SAFE
#endif

// 平台相关的锁类型和函数宏定义
#ifdef RINGBUF_THREAD_SAFE
// FreeRTOS配置 - 嵌入式系统
#if defined(FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
typedef SemaphoreHandle_t ringbuf_mutex_t;
#define RINGBUF_MUTEX_INIT(mutex) (mutex = xSemaphoreCreateMutex())
#define RINGBUF_MUTEX_LOCK(mutex) xSemaphoreTake(mutex, portMAX_DELAY)
#define RINGBUF_MUTEX_UNLOCK(mutex) xSemaphoreGive(mutex)
#define RINGBUF_MUTEX_DESTROY(mutex) vSemaphoreDelete(mutex)

// CMSIS-RTOS配置 - ARM嵌入式系统
#elif defined(CMSIS_OS_H)
#include "cmsis_os.h"
typedef osMutexId ringbuf_mutex_t;
#define RINGBUF_MUTEX_INIT(mutex) (mutex = osMutexNew(NULL))
#define RINGBUF_MUTEX_LOCK(mutex) osMutexAcquire(mutex, osWaitForever)
#define RINGBUF_MUTEX_UNLOCK(mutex) osMutexRelease(mutex)
#define RINGBUF_MUTEX_DESTROY(mutex) osMutexDelete(mutex)

#elif defined(_WIN32) || defined(_WIN64)  // Windows平台
#include <windows.h>
typedef HANDLE ringbuf_mutex_t;
#define RINGBUF_MUTEX_INIT(mutex) (mutex = CreateMutex(NULL, FALSE, NULL))

#define RINGBUF_MUTEX_LOCK(mutex) WaitForSingleObject(mutex, INFINITE)
#define RINGBUF_MUTEX_UNLOCK(mutex) ReleaseMutex(mutex)
#define RINGBUF_MUTEX_DESTROY(mutex) CloseHandle(mutex)

// 自定义锁接口 - 用户需要自己实现这些宏
#elif defined(RINGBUF_CUSTOM_LOCK)
// 用户必须自定义以下宏：
// RINGBUF_MUTEX_INIT, RINGBUF_MUTEX_LOCK, RINGBUF_MUTEX_UNLOCK, RINGBUF_MUTEX_DESTROY
// 并定义 ringbuf_mutex_t 类型

// 无锁配置（单线程）
#else
#undef RINGBUF_THREAD_SAFE
#endif
#endif

// 如果没有定义线程安全或者用户禁用了线程安全，则定义为空操作
#ifndef RINGBUF_THREAD_SAFE
typedef void *ringbuf_mutex_t;
#define RINGBUF_MUTEX_INIT(mutex) (0)
#define RINGBUF_MUTEX_LOCK(mutex) (0)
#define RINGBUF_MUTEX_UNLOCK(mutex) (0)
#define RINGBUF_MUTEX_DESTROY(mutex) (0)
#endif

// ========================= 环形缓冲区结构体 =========================
typedef struct
{
	uint8_t *buffer;        // 指向缓冲区内存
	uint16_t capacity;      // 缓冲区总容量（元素个数）
	uint16_t element_size;  // 每个元素的大小（字节数）
	uint16_t head;          // 写指针（指向下一个写入位置）
	uint16_t tail;          // 读指针（指向下一个读取位置）
	bool full;              // 缓冲区满标志
	ringbuf_mutex_t mutex;  // 互斥锁，用于线程安全
} ringbuf_t;

// ========================= 函数声明 =========================
// 创建和销毁
ringbuf_t *ringbuf_create(uint16_t capacity, uint16_t element_size);
ringbuf_t *ringbuf_create_from_buf(void *buffer, uint16_t capacity, uint16_t element_size, bool is_static);
void ringbuf_free(ringbuf_t *buf);

// 数据操作
bool ringbuf_write_element(ringbuf_t *buf, const void *element);
bool ringbuf_write_elements(ringbuf_t *buf, const void *elements, uint16_t count);
bool ringbuf_read_element(ringbuf_t *buf, void *element);
uint16_t ringbuf_read_elements(ringbuf_t *buf, void *elements, uint16_t count);
bool ringbuf_peek_element(ringbuf_t *buf, void *element);
uint16_t ringbuf_peek_elements(ringbuf_t *buf, void *elements, uint16_t count);
bool ringbuf_push_back(ringbuf_t *buf, const void *element, uint16_t count);
bool ringbuf_back(ringbuf_t *buf, void *element);
bool ringbuf_front(ringbuf_t *buf, void *element);

// 状态查询
uint16_t ringbuf_get_element_count(ringbuf_t *buf);
uint16_t ringbuf_get_free_elements(ringbuf_t *buf);
uint16_t ringbuf_get_byte_count(ringbuf_t *buf);
uint16_t ringbuf_get_free_bytes(ringbuf_t *buf);
bool ringbuf_is_empty(ringbuf_t *buf);
bool ringbuf_is_full(ringbuf_t *buf);

// 工具函数
void ringbuf_clear(ringbuf_t *buf);
uint16_t ringbuf_get_element_size(ringbuf_t *buf);
uint16_t ringbuf_get_capacity(ringbuf_t *buf);

#endif