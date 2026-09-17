#include "list_save.h"
#include <stddef.h>
#include <string.h>

#define LIST_LOCK(list) LIST_MUTEX_LOCK((list)->mutex)
#define LIST_UNLOCK(list) LIST_MUTEX_UNLOCK((list)->mutex)
// 对齐宏：向上对齐到指定边界（ARM需要4字节对齐）
#define ALIGN_UP(size, align) (((size) + (align) - 1) & ~((size_t)(align) - 1))
// 节点大小：结构体大小 + 数据大小，然后对齐到4字节（ARM指针对齐要求）
#define LIST_NODE_SIZE(element_size) ALIGN_UP(sizeof(list_node_t) + ((element_size) > 0 ? (element_size) : 1), 4)

static uint16_t list_node_to_index(list_handle_t list, list_node_t *node)
{
	if (node == NULL || list == NULL || list->node_pool == NULL)
		return 0xFFFF;  // 无效索引

	size_t node_size = LIST_NODE_SIZE(list->element_size);
	ptrdiff_t byte_diff = (uint8_t *)node - (uint8_t *)list->node_pool;

	if (byte_diff < 0 || (byte_diff % node_size) != 0)
		return 0xFFFF;

	uint16_t index = (uint16_t)(byte_diff / node_size);
	if (index >= list->capacity)
		return 0xFFFF;

	return index;
}

static list_node_t *list_index_to_node(list_handle_t list, uint16_t index)
{
	if (list == NULL || list->node_pool == NULL || index >= list->capacity)
		return NULL;

	size_t node_size = LIST_NODE_SIZE(list->element_size);
	return (list_node_t *)((uint8_t *)list->node_pool + index * node_size);
}

static inline size_t list_persist_node_size(uint16_t element_size)
{
	return sizeof(uint16_t) + element_size;  // index + data
}

uint32_t list_get_serialize_size(list_handle_t list)
{
	if (list == NULL)
		return 0;

	// 头部大小（不包含nodes[]）+ 节点数组大小
	// sizeof(list_persist_header_t) 不包含灵活数组成员 nodes[]
	size_t node_persist_size = list_persist_node_size(list->element_size);
	return sizeof(list_persist_header_t) + list->size * node_persist_size;
}

uint32_t list_serialize(list_handle_t list, void *buffer, uint32_t buffer_size)
{
	if (list == NULL || buffer == NULL)
		return 0;

	uint32_t required_size = list_get_serialize_size(list);
	if (buffer_size < required_size)
		return 0;

	LIST_LOCK(list);

	// 填充头部
	list_persist_header_t *header = (list_persist_header_t *)buffer;
	header->size = list->size;
	header->capacity = list->capacity;
	header->element_size = list->element_size;

	// 填充节点数组（按照链表的逻辑顺序，每个节点包含index和data）
	size_t node_persist_size = list_persist_node_size(list->element_size);

	list_node_t *current = list->head;
	uint16_t idx = 0;

	// 节点起始地址（注意：list_persist_node_t包含灵活数组，不能直接用数组索引）
	uint8_t *node_ptr = (uint8_t *)header + sizeof(list_persist_header_t);

	while (current != NULL && idx < list->size)
	{
		uint16_t node_idx = list_node_to_index(list, current);
		if (node_idx == 0xFFFF)
		{
			LIST_UNLOCK(list);
			return 0;  // 错误
		}

		// 手动计算偏移（因为list_persist_node_t包含灵活数组data[]）
		list_persist_node_t *persist_node = (list_persist_node_t *)node_ptr;
		persist_node->index = node_idx;
		memcpy(persist_node->data, current->data, list->element_size);

		// 移动到下一个节点
		node_ptr += node_persist_size;

		current = current->next;
		idx++;
	}

	LIST_UNLOCK(list);
	return required_size;
}
// 反序列化：从缓冲区恢复链表
bool list_deserialize(list_handle_t list, const void *buffer, uint32_t buffer_size)
{
	if (list == NULL || buffer == NULL)
		return false;

	const list_persist_header_t *header = (const list_persist_header_t *)buffer;

	// 验证头部信息
	// 1. 旧链表的size不能超过其capacity
	// 2. 元素大小必须一致
	// 3. 新链表的capacity必须 >= 旧链表的capacity
	if (header->size > header->capacity ||
	    header->element_size != list->element_size ||
	    list->capacity < header->capacity)
		return false;

	// 计算所需大小
	size_t node_persist_size = list_persist_node_size(header->element_size);
	uint32_t required_size = sizeof(list_persist_header_t) +
	                         header->size * node_persist_size;
	if (buffer_size < required_size)
		return false;

	LIST_LOCK(list);

	// 清空链表
	list_clear(list);

	// 重新初始化free_list
	size_t node_size = LIST_NODE_SIZE(list->element_size);
	list->free_list = NULL;
	for (uint16_t i = 0; i < list->capacity; i++)
	{
		list_node_t *node = (list_node_t *)((uint8_t *)list->node_pool + i * node_size);
		node->next = list->free_list;
		node->prev = NULL;
		list->free_list = node;
	}

	// 标记已使用的节点
	bool *node_used = (bool *)calloc(list->capacity, sizeof(bool));
	if (node_used == NULL)
	{
		LIST_UNLOCK(list);
		return false;
	}

	list_node_t *prev_node = NULL;

	// 读取节点数组，list_persist_node_t包含灵活数组，不能直接用数组索引
	const uint8_t *node_ptr = (const uint8_t *)header + sizeof(list_persist_header_t);

	for (uint16_t i = 0; i < header->size; i++)
	{
		// 手动计算偏移，因为list_persist_node_t包含灵活数组data[]
		const list_persist_node_t *persist_node = (const list_persist_node_t *)node_ptr;
		uint16_t node_idx = persist_node->index;

		if (node_idx >= list->capacity || node_used[node_idx])
		{
			free(node_used);
			LIST_UNLOCK(list);
			return false;
		}

		list_node_t *node = list_index_to_node(list, node_idx);
		if (node == NULL)
		{
			free(node_used);
			LIST_UNLOCK(list);
			return false;
		}
		node_used[node_idx] = true;

		// 恢复数据
		memcpy(node->data, persist_node->data, list->element_size);

		// 移动到下一个节点
		node_ptr += node_persist_size;

		// 设置链接关系
		node->prev = prev_node;
		node->next = NULL;

		if (prev_node == NULL)
		{
			list->head = node;
		}
		else
		{
			prev_node->next = node;
		}

		prev_node = node;
		list->size++;
	}

	list->tail = prev_node;

	// 重建free_list（包含未使用的节点）
	list->free_list = NULL;
	for (uint16_t i = 0; i < list->capacity; i++)
	{
		if (!node_used[i])
		{
			list_node_t *node = list_index_to_node(list, i);
			if (node != NULL)
			{
				node->next = list->free_list;
				node->prev = NULL;
				list->free_list = node;
			}
		}
	}

	free(node_used);
	LIST_UNLOCK(list);
	return true;
}