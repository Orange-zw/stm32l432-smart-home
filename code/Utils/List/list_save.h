/**
 * @file list_save.h
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

#ifndef __LIST_SAVE_H__
#define __LIST_SAVE_H__

#include "embedded_list.h"

typedef struct
{
	uint16_t index;
	uint8_t data[];
} list_persist_node_t;

/**
 * @brief 链表持久化头部信息·
 * @note 头部信息后面紧跟着 list_persist_node_t 数组
 */
typedef struct
{
	uint16_t size;          // 当前元素数量
	uint16_t capacity;      // 容量
	uint16_t element_size;  // 元素大小
	                        // 后面跟着 list_persist_node_t 数组（size个元素，每个包含index和data）
	                        // 使用uint8_t[]避免嵌套灵活数组成员的问题（某些编译器不支持）
	uint8_t data[];         // 节点数据区域，通过指针算术访问，不使用list_persist_node_t nodes[]以兼容编译器
} list_persist_header_t;

/**
 * @brief 序列化：将链表保存到缓冲区
 * @param list 链表指针
 * @param buffer 保存缓冲区（需要预先分配足够大小）
 * @param buffer_size 缓冲区大小
 * @return 实际使用的字节数，失败返回0
 */
uint32_t list_serialize(list_handle_t  list, void *buffer, uint32_t buffer_size);

/**
 * @brief 反序列化：从缓冲区恢复链表
 * @param list 已创建的链表（使用list_create或list_create_from_buf）
 * @param buffer 保存的数据缓冲区
 * @param buffer_size 缓冲区大小
 * @return 是否成功
 * @note 新链表的capacity必须 >= 旧链表的capacity，element_size必须一致
 * @note 允许新链表容量大于旧链表，这样可以实现"升级"到更大容量的链表
 */
bool list_deserialize(list_handle_t  list, const void *buffer, uint32_t buffer_size);

/**
 * @brief 计算序列化所需缓冲区大小
 * @param list 链表指针
 * @return 序列化所需缓冲区大小
 */
uint32_t list_get_serialize_size(list_handle_t  list);

#endif
