#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdint.h>
#include "List/embedded_list.h"
#include "Ring_Buf/ring_buf.h"

/* 工具宏定义 */
#define _max(x, y) (x > y ? x : y)
#define _min(x, y) (x < y ? x : y)
#define _map(x, in_min, in_max, out_min, out_max) ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)
#define _constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define _val_in_range(x, min, max) ((x) >= (min) && (x) <= (max))
#define _strcat_fmt(buf, fmt, ...)        \
	do                                    \
	{                                     \
		char tmp[256];                    \
		sprintf(tmp, fmt, ##__VA_ARGS__); \
		strcat(buf, tmp);                 \
	} while (0)

#define _assert(expr)         \
	do                        \
	{                         \
		if (!(expr))          \
		{                     \
			*(uint8_t*)0 = 0; \
		}                     \
	} while (0)

/* 对齐宏 */
#define ALIGN_UP(size, align) (((size) + (align) - 1) & ~((size_t)(align) - 1))  // 向上对齐到指定边界

/* 位操作宏 */
#define BIT_SET(x, y) (x |= (1 << y))
#define BIT_CLEAR(x, y) (x &= ~(1 << y))
#define BIT_TOGGLE(x, y) (x ^= (1 << y))
#define BIT_CHECK(x, y) (x & (1 << y))
#define BIT_SET_MASK(x, y, mask) (x |= (mask))
#define BIT_CLEAR_MASK(x, y, mask) (x &= ~(mask))
#define BIT_TOGGLE_MASK(x, y, mask) (x ^= (mask))
#define BIT_CHECK_MASK(x, y, mask) (x & (mask))

// 获取一个数中1的个数（popcount）
// x: 要统计的值
// 返回：x中1的个数
#if defined(__GNUC__) || defined(__clang__)
// GCC/Clang编译器：使用内置函数（最快，单条指令）
#define BIT_COUNT(x) (__builtin_popcount(x))
// 获取64位整数中1的个数
#define BIT_COUNT_64(x) (__builtin_popcountll(x))
#else
// 其他编译器：使用Brian Kernighan算法
// 算法原理：每次清除最低位的1，直到数为0
static inline int bit_count_impl(unsigned int x)
{
	int count = 0;
	while (x)
	{
		x &= x - 1;  // 清除最低位的1
		count++;
	}
	return count;
}
#define BIT_COUNT(x) (bit_count_impl(x))
#define BIT_COUNT_64(x) (bit_count_impl((unsigned int)(x)) + bit_count_impl((unsigned int)((x) >> 32)))
#endif

// 在mask范围内统计1的个数
// x: 要统计的值，mask: 掩码
// 返回：x & mask 中1的个数
#define BIT_COUNT_MASK(x, mask) (BIT_COUNT((x) & (mask)))

// 获取最低位的1 对应的值
#define LOW_BIT(x) ((x) & (-(x)))

// 获取第一个1的位置（从1开始，0表示没有1）
// x: 要查找的值
#define BIT_GET_POS(x) ((x) ? (BIT_COUNT(LOW_BIT(x) - 1) + 1) : 0)

// 在mask范围内获取第一个1的位置（从1开始，0表示没有1）
// x: 要查找的值，mask: 掩码
#define BIT_GET_POS_MASK(x, mask) (BIT_GET_POS(x & mask))

// 在mask范围内获取第一个1的位置，并加上偏移量offset（从1开始，0表示没有1）
// x: 要查找的值，mask: 掩码，offset: 偏移量
// 注意：__builtin_ffs返回的位置从1开始，所以offset需要减1来正确对齐
// 如果找到1，返回位置+offset-1；如果没找到，返回0
#define BIT_GET_POS_MASK_N(x, mask, offset) \
	(BIT_GET_POS((x) & mask) + (offset) - 1)

/* 时间设置 */
void     Time_check(int16_t* year, int16_t* month, int16_t* day, int16_t* hour, int16_t* min, int16_t* sec);
uint32_t Time_sec(int16_t hour, int16_t min, int16_t sec);
#endif  // __UTILITY_H__