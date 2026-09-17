/**
 * @file embedded_list.h
 * @brief Embedded-List: A high-performance doubly-linked list library for embedded systems
 *
 * This library provides a static node pool-based linked list implementation optimized
 * for embedded systems. It uses flexible array members to embed data directly in nodes,
 * avoiding memory fragmentation and providing predictable performance.
 *
 * Key Features:
 * - Static node pool pre-allocation (no runtime memory fragmentation)
 * - Data embedded in nodes using flexible arrays
 * - Optional thread-safe support with recursive mutexes
 * - Support for both dynamic and static memory allocation
 * - Iterator-based API similar to C++ STL
 * - Zero dependencies (only standard C library)
 *
 * @author DAI
 * @date 2025-12-30
 * @license MIT
 */

#ifndef EMBEDDED_LIST_H
#define EMBEDDED_LIST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// ========================= 线程安全配置 =========================
// 默认禁用线程安全，如需线程安全请在包含头文件前定义 LIST_ENABLE_THREAD_SAFE
// #define LIST_DISABLE_THREAD_SAFE
#ifndef LIST_DISABLE_THREAD_SAFE
#define LIST_THREAD_SAFE
#endif

// 平台相关的递归互斥锁定义
#ifdef LIST_THREAD_SAFE
// FreeRTOS
#if defined(FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
typedef SemaphoreHandle_t list_mutex_t;
#define LIST_MUTEX_INIT(mutex) (mutex = xSemaphoreCreateRecursiveMutex())
#define LIST_MUTEX_LOCK(mutex) xSemaphoreTakeRecursive(mutex, portMAX_DELAY)
#define LIST_MUTEX_UNLOCK(mutex) xSemaphoreGiveRecursive(mutex)
#define LIST_MUTEX_DESTROY(mutex) vSemaphoreDelete(mutex)

// CMSIS-RTOS
#elif defined(CMSIS_OS_H)
#include "cmsis_os.h"
typedef osMutexId list_mutex_t;
#define LIST_MUTEX_INIT(mutex) (mutex = osMutexNew(NULL))
#define LIST_MUTEX_LOCK(mutex) osMutexAcquire(mutex, osWaitForever)
#define LIST_MUTEX_UNLOCK(mutex) osMutexRelease(mutex)
#define LIST_MUTEX_DESTROY(mutex) osMutexDelete(mutex)

#elif defined(_WIN32) || defined(_WIN64)
#include <windows.h>
typedef HANDLE list_mutex_t;
#define LIST_MUTEX_INIT(mutex) (mutex = CreateMutex(NULL, FALSE, NULL))
#define LIST_MUTEX_LOCK(mutex) WaitForSingleObject(mutex, INFINITE)
#define LIST_MUTEX_UNLOCK(mutex) ReleaseMutex(mutex)
#define LIST_MUTEX_DESTROY(mutex) CloseHandle(mutex)

// 自定义锁
#elif defined(LIST_CUSTOM_LOCK)
// 需要自定义的宏：
// LIST_MUTEX_INIT, LIST_MUTEX_LOCK, LIST_MUTEX_UNLOCK, LIST_MUTEX_DESTROY
// 以及 list_mutex_t 类型
// 注意：如果使用自定义锁，请确保实现的是递归锁

// 无锁模式
#else
#undef LIST_THREAD_SAFE
#endif
#endif

// 如果没有线程安全或未定义线程安全，则定义为空操作
#ifndef LIST_THREAD_SAFE
typedef void *list_mutex_t;
#define LIST_MUTEX_INIT(mutex) (0)
#define LIST_MUTEX_LOCK(mutex) (0)
#define LIST_MUTEX_UNLOCK(mutex) (0)
#define LIST_MUTEX_DESTROY(mutex) (0)
#endif

// ========================= 链表结构定义 =========================
typedef struct list_node_t
{
	struct list_node_t *next;    // 指向下一个节点
	struct list_node_t *prev;    // 指向上一个节点
	uint8_t             data[];  // 嵌入的数据（灵活数组成员）
} list_node_t;

typedef struct
{
	list_node_t *head;          // 头节点指针
	list_node_t *tail;          // 尾节点指针
	uint16_t     size;          // 当前元素数量
	uint16_t     capacity;      // 最大容量
	uint16_t     element_size;  // 每个元素的大小（字节）
	list_node_t *free_list;     // 空闲节点链表
	list_node_t *node_pool;     // 节点池
	bool         is_static;     // 是否为静态分配
	list_mutex_t mutex;         // 线程安全互斥锁
} list_t;

typedef list_t      *list_handle_t;    // 链表句柄
typedef list_node_t *list_iterator_t;  // 迭代器

// 比较函数类型
typedef bool (*list_predicate_func_t)(const void *list_data, const void *predicate_data);
typedef void (*list_foreach_func_t)(list_iterator_t it, void *user_data);

// ========================= 创建和销毁 =========================
list_handle_t list_create(uint16_t capacity, uint16_t element_size);
list_handle_t list_create_from_buf(void *data_buf, uint16_t capacity, uint16_t element_size);
void          list_free(list_handle_t list);

// ========================= 容量查询 =========================
bool     list_empty(list_handle_t list);
bool     list_full(list_handle_t list);
uint16_t list_size(list_handle_t list);
uint16_t list_max_size(list_handle_t list);
uint16_t list_capacity(list_handle_t list);
// ========================= 元素访问 =========================

bool            list_front(list_handle_t list, void *element);
bool            list_back(list_handle_t list, void *element);
list_iterator_t list_begin(list_handle_t list);
list_iterator_t list_end(list_handle_t list);
list_iterator_t list_next(list_iterator_t it);
list_iterator_t list_prev(list_iterator_t it);
list_iterator_t list_at(list_handle_t list, int16_t index);
void           *list_get(list_handle_t list, int16_t index);
int16_t         list_index(list_handle_t list, list_iterator_t it);

// ========================= 修改操作 =========================
void list_clear(list_handle_t list);
bool list_insert(list_handle_t list, list_iterator_t position, const void *element);
bool list_erase(list_handle_t list, list_iterator_t position);
bool list_replace(list_handle_t list, list_iterator_t position, const void *element);
bool list_push_front(list_handle_t list, const void *element);
bool list_push_back(list_handle_t list, const void *element);
bool list_pop_front(list_handle_t list, void *element);
bool list_pop_back(list_handle_t list, void *element);
void list_swap(list_handle_t list1, list_handle_t list2);

// ========================= 列表专有操作 =========================
bool     list_splice(list_handle_t list1, list_iterator_t position, list_handle_t list2, list_iterator_t first, list_iterator_t last);
bool     list_merge(list_handle_t list1, list_handle_t list2);
uint16_t list_remove(list_handle_t list, const void *value);
uint16_t list_remove_if(list_handle_t list, list_predicate_func_t predicate, const void *predicate_data);
void     list_reverse(list_handle_t list);
uint16_t list_unique(list_handle_t list);

// ========================= 工具函数 =========================
list_iterator_t list_find(list_handle_t list, const void *value);
list_iterator_t list_find_if(list_handle_t list, list_iterator_t start, list_predicate_func_t predicate, const void *value);
void            list_for_each_if(list_handle_t list, list_foreach_func_t callback, void *user_data);
bool            list_contains(list_handle_t list, const void *value);

#endif