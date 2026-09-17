
#include "embedded_list.h"
#include <stdint.h>
#include <string.h>

#define ALIGN_UP(size, align) (((size) + (align) - 1) & ~((size_t)(align) - 1))
#define LIST_LOCK(list) LIST_MUTEX_LOCK((list)->mutex)
#define LIST_UNLOCK(list) LIST_MUTEX_UNLOCK((list)->mutex)
#define LIST_NODE_SIZE(element_size) ALIGN_UP(sizeof(list_node_t) + ((element_size) > 0 ? (element_size) : 1), 4)

static list_node_t *list_alloc_node(list_handle_t list);
static void         list_free_node(list_handle_t list, list_node_t *node);
static void         list_init_free_list(list_handle_t list);

list_handle_t list_create(uint16_t capacity, uint16_t element_size)
{
	if (capacity == 0 || element_size == 0)
		return NULL;

	list_handle_t list = (list_handle_t)malloc(sizeof(list_t));
	if (list == NULL)
		return NULL;

	// 分配节点池
	size_t node_size = LIST_NODE_SIZE(element_size);
	list->node_pool = (list_node_t *)malloc(capacity * node_size);
	if (list->node_pool == NULL)
	{
		free(list);
		return NULL;
	}

	list->capacity = capacity;
	list->element_size = element_size;
	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
	list->free_list = NULL;
	list->is_static = false;

	// 初始化空闲链表
	list_init_free_list(list);

	// 初始化互斥锁
	LIST_MUTEX_INIT(list->mutex);

	return list;
}

list_handle_t list_create_from_buf(void *node_pool_buf, uint16_t capacity, uint16_t element_size)
{
	if (node_pool_buf == NULL || capacity == 0 || element_size == 0)
		return NULL;

	// 检查缓冲区地址是否4字节对齐
	if (((unsigned long)node_pool_buf & 3) != 0)
		return NULL;

	list_handle_t list = (list_handle_t)malloc(sizeof(list_t));
	if (list == NULL)
		return NULL;

	// 使用外部提供的节点池缓冲区
	list->node_pool = (list_node_t *)node_pool_buf;
	list->capacity = capacity;
	list->element_size = element_size;
	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
	list->free_list = NULL;
	list->is_static = true;

	// 初始化空闲链表
	list_init_free_list(list);

	// 初始化互斥锁
	LIST_MUTEX_INIT(list->mutex);

	return list;
}

static void list_init_free_list(list_handle_t list)
{
	if (list->node_pool == NULL)
		return;

	list->free_list = list->node_pool;
	size_t node_size = LIST_NODE_SIZE(list->element_size);

	for (uint16_t i = 0; i < list->capacity; i++)
	{
		list_node_t *node = (list_node_t *)((uint8_t *)list->node_pool + i * node_size);
		node->next = (i < list->capacity - 1) ? (list_node_t *)((uint8_t *)list->node_pool + (i + 1) * node_size) : NULL;
		node->prev = NULL;
	}
}

void list_free(list_handle_t list)
{
	if (list != NULL)
	{
		LIST_MUTEX_DESTROY(list->mutex);

		if (!list->is_static)
		{
			free(list->node_pool);
		}
		free(list);
	}
}

static list_node_t *list_alloc_node(list_handle_t list)
{
	if (list->free_list == NULL)
		return NULL;

	list_node_t *node = list->free_list;
	list->free_list = node->next;

	// 重置节点状态
	node->next = NULL;
	node->prev = NULL;
	// 清零嵌入的数据
	memset(node->data, 0, list->element_size);

	return node;
}

static void list_free_node(list_handle_t list, list_node_t *node)
{
	if (node == NULL)
		return;

	node->next = list->free_list;
	list->free_list = node;
}

// ========================= 容量查询 =========================
bool list_empty(list_handle_t list)
{
	if (list == NULL)
		return true;

	LIST_LOCK(list);
	bool empty = (list->size == 0);
	LIST_UNLOCK(list);
	return empty;
}

bool list_full(list_handle_t list)
{
	if (list == NULL)
		return false;

	LIST_LOCK(list);
	bool full = (list->size == list->capacity);
	LIST_UNLOCK(list);
	return full;
}

uint16_t list_size(list_handle_t list)
{
	return list ? list->size : 0;
}

uint16_t list_max_size(list_handle_t list)
{
	return list ? list->capacity : 0;
}

uint16_t list_capacity(list_handle_t list)
{
	return list ? list->capacity : 0;
}

// ========================= 元素访问 =========================
bool list_front(list_handle_t list, void *element)
{
	if (list == NULL || element == NULL || list->head == NULL)
		return false;

	LIST_LOCK(list);
	memcpy(element, list->head->data, list->element_size);
	LIST_UNLOCK(list);
	return true;
}

bool list_back(list_handle_t list, void *element)
{
	if (list == NULL || element == NULL || list->tail == NULL)
		return false;

	LIST_LOCK(list);
	memcpy(element, list->tail->data, list->element_size);
	LIST_UNLOCK(list);
	return true;
}

list_iterator_t list_begin(list_handle_t list)
{
	return list ? list->head : NULL;
}

list_iterator_t list_end(list_handle_t list)
{
	return list ? list->tail : NULL;
}

list_iterator_t list_next(list_iterator_t it)
{
	return it ? it->next : NULL;
}

list_iterator_t list_prev(list_iterator_t it)
{
	return it ? it->prev : NULL;
}

/**
 *@brief    获取列表中指定索引的元素
 *@param    list 列表指针
 *@param    index 索引
 *@note     如果索引为负数，则从末尾开始计数
 *@note     如果索引越界，则返回NULL
 *@return   元素指针
 */
list_iterator_t list_at(list_handle_t list, int16_t index)
{
	if (list == NULL || index >= list->size)
		return NULL;

	list_iterator_t current = NULL;
	if (index < 0)
	{
		current = list->tail;
		for (int16_t i = 0; i > index; i--)
			current = list_prev(current);
	}
	else
	{
		current = list->head;
		for (int16_t i = 0; i < index; i++)
			current = list_next(current);
	}
	return current;
}

void *list_get(list_handle_t list, int16_t index)
{
	list_iterator_t it = list_at(list, index);
	return it ? it->data : NULL;
}

/**
 *@brief    获取列表中指定节点的索引
 *@param    list 列表指针
 *@param    it 节点迭代器
 *@return   节点索引，如果节点为NULL，则返回0
 */
int16_t list_index(list_handle_t list, list_iterator_t it)
{
	if (list == NULL || it == NULL)
		return -1;

	int16_t index = 0;
	while (it != list->head)
	{
		it = list_prev(it);
		index++;
	}

	return index;
}

// ========================= 修改操作 =========================
void list_clear(list_handle_t list)
{
	if (list == NULL)
		return;

	LIST_LOCK(list);

	list_node_t *current = list->head;
	while (current != NULL)
	{
		list_node_t *next = current->next;
		list_free_node(list, current);
		current = next;
	}

	list->head = NULL;
	list->tail = NULL;
	list->size = 0;

	LIST_UNLOCK(list);
}

/**
 *@brief    插入元素到列表中
 *@param    list 列表指针
 *@param    position 插入位置
 *@param    element 插入元素
 *@return   是否插入成功
 */
bool list_insert(list_handle_t list, list_iterator_t position, const void *element)
{
	if (list == NULL || element == NULL || list->size >= list->capacity)
		return false;

	LIST_LOCK(list);

	list_node_t *new_node = list_alloc_node(list);
	if (new_node == NULL)
	{
		LIST_UNLOCK(list);
		return false;
	}

	// 复制数据
	memcpy(new_node->data, element, list->element_size);

	if (position == NULL)
	{
		// 插入到末尾
		if (list->tail == NULL)
		{
			// 空链表
			list->head = new_node;
			list->tail = new_node;
		}
		else
		{
			list->tail->next = new_node;
			new_node->prev = list->tail;
			list->tail = new_node;
		}
	}
	else
	{
		// 插入到position之前
		new_node->next = position;
		new_node->prev = position->prev;

		if (position->prev != NULL)
		{
			position->prev->next = new_node;
		}
		else
		{
			// 插入到头部
			list->head = new_node;
		}
		position->prev = new_node;
	}

	list->size++;
	LIST_UNLOCK(list);
	return true;
}

bool list_erase(list_handle_t list, list_iterator_t position)
{
	if (list == NULL || position == NULL)
		return false;

	LIST_LOCK(list);

	if (position->prev != NULL)
	{
		position->prev->next = position->next;
	}
	else
	{
		list->head = position->next;
	}

	if (position->next != NULL)
	{
		position->next->prev = position->prev;
	}
	else
	{
		list->tail = position->prev;
	}

	list_free_node(list, position);
	list->size--;

	LIST_UNLOCK(list);
	return true;
}

bool list_replace(list_handle_t list, list_iterator_t position, const void *element)
{
	if (list == NULL || position == NULL || element == NULL)
		return false;

	LIST_LOCK(list);
	memcpy(position->data, element, list->element_size);
	LIST_UNLOCK(list);
	return true;
}

bool list_push_front(list_handle_t list, const void *element)
{
	return list_insert(list, list->head, element);
}

bool list_push_back(list_handle_t list, const void *element)
{
	return list_insert(list, NULL, element);
}

bool list_pop_front(list_handle_t list, void *element)
{
	if (list == NULL || list->head == NULL)
		return false;

	if (element != NULL)
	{
		memcpy(element, list->head->data, list->element_size);
	}

	return list_erase(list, list->head);
}

bool list_pop_back(list_handle_t list, void *element)
{
	if (list == NULL || list->tail == NULL)
		return false;

	if (element != NULL)
	{
		memcpy(element, list->tail->data, list->element_size);
	}

	return list_erase(list, list->tail);
}

void list_swap(list_handle_t list1, list_handle_t list2)
{
	if (list1 == NULL || list2 == NULL)
		return;

	LIST_LOCK(list1);
	LIST_LOCK(list2);

	// 交换所有成员
	list_node_t *temp_node = list1->head;
	list1->head = list2->head;
	list2->head = temp_node;

	temp_node = list1->tail;
	list1->tail = list2->tail;
	list2->tail = temp_node;

	temp_node = list1->free_list;
	list1->free_list = list2->free_list;
	list2->free_list = temp_node;

	uint16_t temp_size = list1->size;
	list1->size = list2->size;
	list2->size = temp_size;

	LIST_UNLOCK(list2);
	LIST_UNLOCK(list1);
}

// ========================= 列表专有操作 =========================
/**
 *
 * @brief    list_splice 将 list2 的 [first, last) 节点移动到 list1 的 position 之前。
 * @note     如果 position 为 NULL，则移动到 list1 的末尾。
 * @note     如果 first 或 last 为 NULL，则移动 list2 的所有节点。
 * @note     如果 list1 和 list2 的元素大小不一致，则返回 false。
 * @note     如果移动的节点数量大于 list1 的容量，则返回 false。
 * @note     如果移动的节点数量大于 list2 的容量，则返回 false。
 * @note     如果移动的节点数量大于 list1 的容量，则返回 false。·
 * @param    list1 目标列表
 * @param    position 插入位置
 * @param    list2 源列表
 * @param    first 开始节点
 * @param    last 结束节点
 * @return   是否移动成功
 */
bool list_splice(list_handle_t list1, list_iterator_t position, list_handle_t list2,
                 list_iterator_t first, list_iterator_t last)
{
	if (list1 == NULL || list2 == NULL || first == NULL)
		return false;

	if (list1->element_size != list2->element_size)
		return false;

	LIST_LOCK(list1);
	LIST_LOCK(list2);

	// 计算要移动的节点数量
	uint16_t        move_count = 0;
	list_iterator_t it = first;
	while (it != last && it != NULL)
	{
		move_count++;
		it = it->next;
	}

	// 检查容量
	if (list1->size + move_count > list1->capacity)
	{
		LIST_UNLOCK(list2);
		LIST_UNLOCK(list1);
		return false;
	}

	// 从list2中移除节点段
	list_iterator_t before_first = first->prev;
	list_iterator_t after_last = (last != NULL) ? last : list2->tail;

	list_iterator_t last_of_segment = NULL;
	if (last != NULL)
	{
		last_of_segment = last->prev;  // 保存移动段的最后一个节点
	}
	else
	{
		// 如果 last == NULL，移动段一直到末尾
		list_iterator_t temp = first;
		while (temp != NULL && temp->next != NULL)
			temp = temp->next;
		last_of_segment = temp;  // 移动段的最后一个节点
	}

	if (before_first != NULL)
	{
		before_first->next = after_last;
	}
	else
	{
		list2->head = after_last;
	}

	if (after_last != NULL)
	{
		after_last->prev = before_first;
	}
	else
	{
		list2->tail = before_first;
	}

	// 调整要移动的节点段
	first->prev = NULL;
	if (last != NULL)
	{
		if (last_of_segment != NULL)
		{
			last_of_segment->next = NULL;  // 断开移动段的尾部
		}
	}

	// 插入到list1
	if (position == NULL)
	{
		// 插入到末尾
		if (list1->tail == NULL)
		{
			list1->head = first;
		}
		else
		{
			list1->tail->next = first;
			first->prev = list1->tail;
		}
		list1->tail = last_of_segment;
	}
	else
	{
		// 插入到position之前
		list_iterator_t before_position = position->prev;

		if (before_position != NULL)
		{
			before_position->next = first;
			first->prev = before_position;
		}
		else
		{
			list1->head = first;
		}

		position->prev = last_of_segment;
		if (last_of_segment != NULL)
		{
			last_of_segment->next = position;
		}
	}

	list1->size += move_count;
	list2->size -= move_count;

	LIST_UNLOCK(list2);
	LIST_UNLOCK(list1);
	return true;
}

bool list_merge(list_handle_t list1, list_handle_t list2)
{
	return list_splice(list1, NULL, list2, list2->head, list2->tail);
}

/**
 *@brief    删除列表中的指定元素
 *@param    list 列表指针
 *@param    value 要删除的元素
 *@return   删除的元素数量
 */
uint16_t list_remove(list_handle_t list, const void *value)
{
	if (list == NULL || value == NULL)
		return 0;

	return list_remove_if(list, NULL, value);
}

/**
 *@brief    删除列表中的符合谓词函数的元素
 *@param    list 列表指针
 *@param    predicate 谓词函数指针
 *@param    predicate_data 谓词函数参数
 *@return   删除的元素数量
 */
uint16_t list_remove_if(list_handle_t list, list_predicate_func_t predicate, const void *predicate_data)
{
	if (list == NULL)
		return 0;

	LIST_LOCK(list);

	uint16_t     remove_count = 0;
	list_node_t *current = list->head;
	list_node_t *next;

	while (current != NULL)
	{
		next = current->next;

		uint8_t res = predicate ? predicate(current->data, predicate_data) : memcmp(current->data, predicate_data, list->element_size) == 0;
		if (res)
		{
			list_erase(list, current);
			remove_count++;
		}

		current = next;
	}

	LIST_UNLOCK(list);
	return remove_count;
}

void list_reverse(list_handle_t list)
{
	if (list == NULL)
		return;

	LIST_LOCK(list);

	list_node_t *current = list->head;
	list_node_t *temp = NULL;

	while (current != NULL)
	{
		// 交换prev和next指针
		temp = current->prev;
		current->prev = current->next;
		current->next = temp;

		current = current->prev;  // 移动到原下一个节点
	}

	// 交换头尾指针
	temp = list->head;
	list->head = list->tail;
	list->tail = temp;

	LIST_UNLOCK(list);
}

/**
 *@brief    删除列表中的重复元素
 *@param    list 列表指针
 *@return   删除的元素数量
 *@note     这个函数的时间复杂度是O(n^2)，需要遍历所有元素，不适合用于大型列表，适合链表长度较小或内存受限的嵌入式系统
 *@note     如果需要优化时间复杂度，可以考虑使用哈希表实现。
 */
uint16_t list_unique(list_handle_t list)
{
	if (list == NULL)
		return 0;

	LIST_LOCK(list);

	uint16_t        remove_count = 0;
	list_iterator_t pre = list->head;
	list_iterator_t current = NULL;

	while (pre != NULL)
	{
		current = list_next(pre);
		while (current != NULL)
		{
			if (memcmp(pre->data, current->data, list->element_size) == 0)
			{
				list_node_t *to_remove = current;
				current = list_next(current);
				list_erase(list, to_remove);
				remove_count++;
			}
			else
			{
				current = list_next(current);
			}
		}
		pre = list_next(pre);
	}

	LIST_UNLOCK(list);
	return remove_count;
}

// ========================= 工具函数 =========================

list_iterator_t list_find(list_handle_t list, const void *value)
{
	if (list == NULL || value == NULL)
		return NULL;

	return list_find_if(list, NULL, NULL, value);
}

/**
 *@brief    从指定位置开始查找下一个符合条件的节点
 *@param    list 列表指针
 *@param    start 开始查找的位置（不包含此节点），如果为NULL则从头开始
 *@param    predicate 谓词函数指针，如果为NULL则使用memcmp比较
 *@param    value 要查找的值或谓词函数参数
 *@return   找到的节点迭代器，如果未找到则返回NULL
 */
list_iterator_t list_find_if(list_handle_t list, list_iterator_t start, list_predicate_func_t predicate, const void *value)
{
	if (list == NULL)
		return NULL;

	// 如果使用 memcmp 比较（predicate 为 NULL），则 value 不能为 NULL
	if (predicate == NULL && value == NULL)
		return NULL;

	LIST_LOCK(list);

	list_node_t *current = (start != NULL) ? start->next : list->head;
	while (current != NULL)
	{
		uint8_t res = predicate ? predicate(current->data, value) : memcmp(current->data, value, list->element_size) == 0;
		if (res)
		{
			LIST_UNLOCK(list);
			return current;
		}
		current = current->next;
	}

	LIST_UNLOCK(list);
	return NULL;
}

/**
 *@brief    对列表中所有符合条件的节点执行回调函数
 *@param    list 列表指针
 *@param    predicate 谓词函数指针，如果为NULL则匹配所有节点
 *@param    predicate_data 谓词函数参数
 *@param    callback 回调函数指针
 *@param    user_data 传递给回调函数的用户数据
 *@return   处理的节点数量
 */
void list_for_each_if(list_handle_t list, list_foreach_func_t callback, void *user_data)
{
	if (list == NULL || callback == NULL)
		return;

	LIST_LOCK(list);
	list_node_t *current = list->head;
	while (current != NULL)
	{

		callback(current, user_data);

		current = current->next;
	}

	LIST_UNLOCK(list);
}

bool list_contains(list_handle_t list, const void *value)
{
	return list_find(list, value) != NULL;
}