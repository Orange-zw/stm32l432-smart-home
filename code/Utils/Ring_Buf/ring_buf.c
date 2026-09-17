#include "ring_buf.h"
#include <stdlib.h>
#include <string.h>

// 内部锁操作宏
#define RINGBUF_LOCK(buf) RINGBUF_MUTEX_LOCK((buf)->mutex)
#define RINGBUF_UNLOCK(buf) RINGBUF_MUTEX_UNLOCK((buf)->mutex)

// 创建环形缓冲区
ringbuf_t *ringbuf_create(uint16_t capacity, uint16_t element_size)
{
	if (capacity == 0 || element_size == 0)
		return NULL;

	ringbuf_t *buf = (ringbuf_t *)malloc(sizeof(ringbuf_t));
	if (buf == NULL)
		return NULL;

	// 计算总字节数
	size_t total_bytes = (size_t)capacity * element_size;
	buf->buffer = (uint8_t *)malloc(total_bytes);
	if (buf->buffer == NULL)
	{
		free(buf);
		return NULL;
	}

	buf->capacity = capacity;
	buf->element_size = element_size;
	buf->head = 0;
	buf->tail = 0;
	buf->full = false;

	// 初始化互斥锁
	RINGBUF_MUTEX_INIT(buf->mutex);

	return buf;
}

// 创建环形缓冲区（使用外部缓冲区）
ringbuf_t *ringbuf_create_from_buf(void *buffer, uint16_t capacity, uint16_t element_size, bool is_static)
{
	if (buffer == NULL || capacity == 0 || element_size == 0)
		return NULL;

	ringbuf_t *buf = (ringbuf_t *)malloc(sizeof(ringbuf_t));
	if (buf == NULL)
		return NULL;

	if (is_static)
	{
		buf->buffer = (uint8_t *)buffer;
	}
	else
	{
		// 计算总字节数
		size_t total_bytes = (size_t)capacity * element_size;
		buf->buffer = (uint8_t *)malloc(total_bytes);
		if (buf->buffer == NULL)
		{
			free(buf);
			return NULL;
		}
		memcpy(buf->buffer, buffer, total_bytes);
	}
	buf->capacity = capacity;
	buf->element_size = element_size;
	buf->head = 0;
	buf->tail = 0;
	buf->full = false;

	// 初始化互斥锁
	RINGBUF_MUTEX_INIT(buf->mutex);

	return buf;
}

// 销毁缓冲区并释放内存
void ringbuf_free(ringbuf_t *buf)
{
	if (buf != NULL)
	{
		RINGBUF_MUTEX_DESTROY(buf->mutex);
		free(buf->buffer);
		free(buf);
	}
}

// ========================= 按元素操作 =========================

// 写入单个元素
bool ringbuf_write_element(ringbuf_t *buf, const void *element)
{
	if (buf == NULL || element == NULL || ringbuf_is_full(buf))
		return false;

	RINGBUF_LOCK(buf);

	// 计算元素在缓冲区中的位置
	uint8_t *dest = buf->buffer + (buf->head * buf->element_size);
	memcpy(dest, element, buf->element_size);

	// 更新头指针
	buf->head = (buf->head + 1) % buf->capacity;
	buf->full = (buf->head == buf->tail);

	RINGBUF_UNLOCK(buf);
	return true;
}

// 写入多个元素
bool ringbuf_write_elements(ringbuf_t *buf, const void *elements, uint16_t count)
{
	if (buf == NULL || elements == NULL || count == 0)
		return false;

	RINGBUF_LOCK(buf);

	const uint8_t *src = (const uint8_t *)elements;

	for (uint16_t i = 0; i < count; i++)
	{
		if (buf->full)
		{
			// 缓冲区满，覆盖最旧的数据
			buf->tail = (buf->tail + 1) % buf->capacity;
		}

		// 计算目标位置
		uint8_t *dest = buf->buffer + (buf->head * buf->element_size);
		memcpy(dest, src + (i * buf->element_size), buf->element_size);

		// 更新头指针
		buf->head = (buf->head + 1) % buf->capacity;
		buf->full = (buf->head == buf->tail);
	}

	RINGBUF_UNLOCK(buf);
	return true;
}

// 读取单个元素
bool ringbuf_read_element(ringbuf_t *buf, void *element)
{
	if (buf == NULL || element == NULL || ringbuf_is_empty(buf))
		return false;

	RINGBUF_LOCK(buf);

	// 计算元素在缓冲区中的位置
	const uint8_t *src = buf->buffer + (buf->tail * buf->element_size);
	memcpy(element, src, buf->element_size);

	// 更新尾指针
	buf->tail = (buf->tail + 1) % buf->capacity;
	buf->full = false;
	RINGBUF_UNLOCK(buf);
	return true;
}

// 读取多个元素
uint16_t ringbuf_read_elements(ringbuf_t *buf, void *elements, uint16_t count)
{
	if (buf == NULL || elements == NULL || count == 0)
		return 0;

	uint8_t *dest = (uint8_t *)elements;
	uint16_t read_count = 0;
	uint16_t temp_tail = buf->tail;  // 临时指针，保存当前位置

	while (read_count < count && (!ringbuf_is_empty(buf)))
	{
		if (ringbuf_read_element(buf, dest + (read_count * buf->element_size)))
		{
			read_count++;
		}
	}
	return read_count;
}

bool ringbuf_peek_element(ringbuf_t *buf, void *element)
{
	if (buf == NULL || element == NULL || ringbuf_is_empty(buf))
		return false;

	RINGBUF_LOCK(buf);
	const uint8_t *src = buf->buffer + (buf->tail * buf->element_size);
	memcpy(element, src, buf->element_size);
	RINGBUF_UNLOCK(buf);
	return true;
}

uint16_t ringbuf_peek_elements(ringbuf_t *buf, void *elements, uint16_t count)
{
	if (buf == NULL || elements == NULL || count == 0)
		return 0;

	uint8_t *dest = (uint8_t *)elements;
	uint16_t peek_count = 0;
	uint16_t temp_tail = buf->tail;
	while (peek_count < count && (!ringbuf_is_empty(buf)))
	{
		if (ringbuf_peek_element(buf, dest + (peek_count * buf->element_size)))
		{
			peek_count++;
		}
	}
	return peek_count;
}

bool ringbuf_push_back(ringbuf_t *buf, const void *element, uint16_t count)
{
	if (buf == NULL || element == NULL || count == 0)
		return false;

	return ringbuf_write_elements(buf, element, count);
}

bool ringbuf_back(ringbuf_t *buf, void *element)
{
	if (buf == NULL || element == NULL || ringbuf_is_empty(buf))
		return false;

	memcpy(element, &buf->buffer[buf->tail], buf->element_size);
	return true;
}

bool ringbuf_front(ringbuf_t *buf, void *element)
{
	if (buf == NULL || element == NULL || ringbuf_is_empty(buf))
		return false;

	memcpy(element, &buf->buffer[buf->head], buf->element_size);
	return true;
}

// ========================= 状态查询 =========================

// 获取缓冲区中元素个数
uint16_t ringbuf_get_element_count(ringbuf_t *buf)
{
	if (buf == NULL)
		return 0;

	RINGBUF_LOCK(buf);

	uint16_t count;
	if (buf->full)
	{
		count = buf->capacity;
	}
	else if (buf->head >= buf->tail)
	{
		count = buf->head - buf->tail;
	}
	else
	{
		count = buf->capacity - buf->tail + buf->head;
	}

	RINGBUF_UNLOCK(buf);
	return count;
}

// 获取缓冲区空闲元素个数
uint16_t ringbuf_get_free_elements(ringbuf_t *buf)
{
	if (buf == NULL)
		return 0;

	RINGBUF_LOCK(buf);
	uint16_t free_elements = buf->capacity - ringbuf_get_element_count(buf);
	RINGBUF_UNLOCK(buf);
	return free_elements;
}

// 获取缓冲区字节数（向后兼容）
uint16_t ringbuf_get_byte_count(ringbuf_t *buf)
{
	return ringbuf_get_element_count(buf) * buf->element_size;
}

// 获取缓冲区空闲字节数（向后兼容）
uint16_t ringbuf_get_free_bytes(ringbuf_t *buf)
{
	return ringbuf_get_free_elements(buf) * buf->element_size;
}

// 检查缓冲区是否为空
bool ringbuf_is_empty(ringbuf_t *buf)
{
	if (buf == NULL)
		return true;

	RINGBUF_LOCK(buf);
	bool empty = (!buf->full) && (buf->head == buf->tail);
	RINGBUF_UNLOCK(buf);
	return empty;
}

// 检查缓冲区是否已满
bool ringbuf_is_full(ringbuf_t *buf)
{
	if (buf == NULL)
		return false;

	RINGBUF_LOCK(buf);
	bool full = buf->full;
	RINGBUF_UNLOCK(buf);
	return full;
}

// 清空缓冲区
void ringbuf_clear(ringbuf_t *buf)
{
	if (buf == NULL)
		return;

	RINGBUF_LOCK(buf);
	buf->head = 0;
	buf->tail = 0;
	buf->full = false;
	RINGBUF_UNLOCK(buf);
}

// 获取元素大小
uint16_t ringbuf_get_element_size(ringbuf_t *buf)
{
	return buf ? buf->element_size : 0;
}

// 获取缓冲区容量（元素个数）
uint16_t ringbuf_get_capacity(ringbuf_t *buf)
{
	return buf ? buf->capacity : 0;
}