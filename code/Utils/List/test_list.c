#include "test_list.h"
#include <time.h>
// 在 test_list.c 文件开头添加头文件
#include "list_save.h"
#ifdef _WIN32
#include <windows.h>  // 用于 Sleep 函数
#else
#include <unistd.h>  // Linux/Mac 使用 sleep
#define Sleep(x) sleep((x) / 1000)
#endif

// 测试用例计数器
static int tests_passed = 0;
static int tests_failed = 0;

// 比较函数
bool compare_int(const void *a, const void *b)
{
	int int_a = *(const int *)a;
	int int_b = *(const int *)b;
	return int_a == int_b;
}

bool compare_int_less(const void *a, const void *b)
{
	int int_a = *(const int *)a;
	int int_b = *(const int *)b;
	return int_a <= int_b;
}

bool is_even(const void *list_data, const void *predicate_data)
{
	int value = *(const int *)list_data;
	return (value % 2) == 0;
}

bool is_positive(const void *data)
{
	int value = *(const int *)data;
	return value > 0;
}

typedef struct
{
	int arr[10];
	int count;
} collect_data_t;
void collect_even(list_iterator_t it, void *user_data)
{
	collect_data_t *data = (collect_data_t *)user_data;
	int value = *(int *)it->data;
	// 在回调中过滤偶数
	if (value % 2 == 0 && data->count < 10)
	{
		data->arr[data->count] = value;
		data->count++;
	}
}

void count_all(list_iterator_t it, void *user_data)
{
	(*(int *)user_data)++;
}

void count_target(list_iterator_t it, void *user_data)
{
	int *count = (int *)user_data;
	int value = *(int *)it->data;
	if (value == 5)
	{
		(*count)++;
	}
}

typedef struct
{
	int arr[10];
	int count;
	int threshold;
} filter_data_t;
void collect_greater(list_iterator_t it, void *user_data)
{
	filter_data_t *data = (filter_data_t *)user_data;
	int value = *(int *)it->data;
	if (value > data->threshold && data->count < 10)
	{
		data->arr[data->count] = value;
		data->count++;
	}
}

// 打印测试结果
void print_test_result(test_result_t result)
{
	if (result.passed)
	{
		printf("√ PASS: %s\n", result.test_name);
		tests_passed++;
	}
	else
	{
		printf("× FAIL: %s - %s\n", result.test_name, result.message);
		tests_failed++;
	}
}

// ========================= 具体测试用例 =========================

test_result_t test_list_creation(void)
{
	test_result_t result = {"列表创建和销毁", true, ""};

	// 测试动态创建
	list_handle_t list = list_create(10, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "动态创建失败";
		return result;
	}

	// 验证初始状态
	if (!list_empty(list))
	{
		result.passed = false;
		result.message = "新列表应该为空";
		list_free(list);
		return result;
	}

	if (list_size(list) != 0)
	{
		result.passed = false;
		result.message = "新列表大小应该为0";
		list_free(list);
		return result;
	}

	if (list_capacity(list) != 10)
	{
		result.passed = false;
		result.message = "容量设置错误";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_static_creation(void)
{
	list_iterator_t it;
	test_result_t result = {"静态列表创建", true, ""};

	// 准备静态缓冲区（节点池，包含嵌入的数据）
	typedef struct
	{
		char id[4];
		int value;
		float score;
	} Test_data_t;

	// 计算节点大小（包含嵌入的数据）：sizeof(list_node_t) + element_size
	// list_node_t 只包含 next, prev 指针，data 是灵活数组成员（不计入 sizeof）
	size_t node_size = sizeof(void *) * 2 + sizeof(Test_data_t);  // next + prev + data数组
	uint16_t capacity = 4;                                        // 使用固定容量
	uint8_t node_pool[256];                                       // 足够大的缓冲区用于节点池（4个节点 * 节点大小，256足够）

	printf("Test_data_t size: %zu bytes, node_size: %zu bytes\n", sizeof(Test_data_t), node_size);
	printf("capacity: %d, total pool size: %zu bytes\n", capacity, capacity * node_size);
	list_handle_t list = list_create_from_buf(node_pool, capacity, sizeof(Test_data_t));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "静态创建失败";
		return result;
	}

	printf("Static List created with capacity %d and element size %d\n", list_capacity(list), list->element_size);

	// 测试基本操作
	Test_data_t element[] = {
	    {.id = "A01", .value = 42, .score = 95.5},
	    {.id = "B02", .value = 73, .score = 88.0},
	    {.id = "C03", .value = 15, .score = 76.5},
	};
	for (int i = 0; i < 3; i++)
	{
		if (!list_push_back(list, &element[i]))
		{
			result.passed = false;
			result.message = "静态列表插入失败";
			list_free(list);
			return result;
		}
	}

	printf("Static List size after insertions: %d\n", list_size(list));

	Test_data_t retrieved;
	if (!list_front(list, &retrieved) || retrieved.value != 42)
	{
		result.passed = false;
		result.message = "静态列表数据错误";
		return result;
	}

	it = list_begin(list);
	while (it)
	{
		Test_data_t *data = (Test_data_t *)it->data;
		printf("ID: %s, Value: %d, Score: %.2f\n", data->id, data->value, data->score);
		it = list_next(it);
	}
	printf("\n");
	list_free(list);
	return result;
}

test_result_t test_list_basic_operations(void)
{
	test_result_t result = {"基本操作测试", true, ""};

	list_handle_t list = list_create(5, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 测试空列表状态
	if (!list_empty(list))
	{
		result.passed = false;
		result.message = "新列表应该为空";
		list_free(list);
		return result;
	}

	// 测试大小和容量
	if (list_size(list) != 0 || list_max_size(list) != 5)
	{
		result.passed = false;
		result.message = "大小或容量错误";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_push_pop(void)
{
	test_result_t result = {"压入弹出操作", true, ""};

	list_handle_t list = list_create(5, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 测试push_front和push_back
	int values[] = {1, 2, 3, 4, 5};

	for (int i = 0; i < 5; i++)
	{
		if (!list_push_back(list, &values[i]))
		{
			result.passed = false;
			result.message = "push_back失败";
			list_free(list);
			return result;
		}
	}

	// 测试已满列表
	int extra = 6;
	if (list_push_back(list, &extra))
	{
		result.passed = false;
		result.message = "已满列表应该拒绝插入";
		list_free(list);
		return result;
	}

	// 测试pop_front
	int popped;
	if (!list_pop_front(list, &popped) || popped != 1)
	{
		result.passed = false;
		result.message = "pop_front数据错误";
		list_free(list);
		return result;
	}

	// 测试pop_back
	if (!list_pop_back(list, &popped) || popped != 5)
	{
		result.passed = false;
		result.message = "pop_back数据错误";
		list_free(list);
		return result;
	}

	// 验证剩余大小
	if (list_size(list) != 3)
	{
		result.passed = false;
		result.message = "弹出后大小错误";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_insert_erase(void)
{
	test_result_t result = {"插入删除操作", true, ""};

	list_handle_t list = list_create(10, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 准备测试数据
	int values[] = {1, 3, 5};
	for (int i = 0; i < 3; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 在第二个位置插入2
	list_iterator_t it = list_begin(list);
	it = list_next(it);  // 移动到第二个元素(3)

	int insert_value = 2;
	if (!list_insert(list, it, &insert_value))
	{
		result.passed = false;
		result.message = "插入失败";
		list_free(list);
		return result;
	}

	// 验证插入结果
	int expected[] = {1, 2, 3, 5};
	list_iterator_t current = list_begin(list);
	for (int i = 0; i < 4; i++)
	{
		if (*(int *)current->data != expected[i])
		{
			result.passed = false;
			result.message = "插入后数据错误";
			list_free(list);
			return result;
		}
		current = list_next(current);
	}

	// 测试删除
	it = list_begin(list);
	it = list_next(it);  // 移动到第二个元素(2)
	if (!list_erase(list, it))
	{
		result.passed = false;
		result.message = "删除失败";
		list_free(list);
		return result;
	}

	// 验证删除结果
	if (list_size(list) != 3)
	{
		result.passed = false;
		result.message = "删除后大小错误";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_front_back(void)
{
	test_result_t result = {"前后元素访问", true, ""};

	list_handle_t list = list_create(5, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 测试空列表访问
	int dummy;
	if (list_front(list, &dummy) || list_back(list, &dummy))
	{
		result.passed = false;
		result.message = "空列表访问应该失败";
		list_free(list);
		return result;
	}

	// 添加数据测试
	int values[] = {10, 20, 30};
	for (int i = 0; i < 3; i++)
	{
		list_push_back(list, &values[i]);
	}

	int front, back;
	if (!list_front(list, &front) || front != 10)
	{
		result.passed = false;
		result.message = "front数据错误";
		list_free(list);
		return result;
	}

	if (!list_back(list, &back) || back != 30)
	{
		result.passed = false;
		result.message = "back数据错误";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_clear(void)
{
	test_result_t result = {"清空操作", true, ""};

	list_handle_t list = list_create(5, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 添加一些数据
	int values[] = {1, 2, 3, 4, 5};
	for (int i = 0; i < 5; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 清空列表
	list_clear(list);

	if (!list_empty(list) || list_size(list) != 0)
	{
		result.passed = false;
		result.message = "清空后列表应该为空";
		list_free(list);
		return result;
	}

	// 测试清空后能否重新添加
	int new_value = 100;
	if (!list_push_back(list, &new_value))
	{
		result.passed = false;
		result.message = "清空后无法添加新元素";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_replace(void)
{
	test_result_t result = {"替换操作", true, ""};

	list_handle_t list = list_create(5, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	int values[] = {1, 2, 3};
	for (int i = 0; i < 3; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 替换第二个元素
	list_iterator_t it = list_begin(list);
	it = list_next(it);
	int new_value = 99;

	if (!list_replace(list, it, &new_value))
	{
		result.passed = false;
		result.message = "替换失败";
		list_free(list);
		return result;
	}

	// 验证替换结果
	int retrieved;
	it = list_begin(list);
	it = list_next(it);
	if (*(int *)it->data != 99)
	{
		result.passed = false;
		result.message = "替换后数据错误";
		list_free(list);
		return result;
	}

	// 验证列表大小不变
	if (list_size(list) != 3)
	{
		result.passed = false;
		result.message = "替换不应该改变大小";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_swap(void)
{
	test_result_t result = {"交换操作", true, ""};

	list_handle_t list1 = list_create(5, sizeof(int));
	list_handle_t list2 = list_create(3, sizeof(int));

	if (list1 == NULL || list2 == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		if (list1)
			list_free(list1);
		if (list2)
			list_free(list2);
		return result;
	}

	// 添加测试数据
	int values1[] = {1, 2, 3};
	int values2[] = {4, 5};

	for (int i = 0; i < 3; i++)
		list_push_back(list1, &values1[i]);
	for (int i = 0; i < 2; i++)
		list_push_back(list2, &values2[i]);

	// 记录交换前状态
	uint16_t size1_before = list_size(list1);
	uint16_t size2_before = list_size(list2);

	// 执行交换
	list_swap(list1, list2);

	// 验证交换结果
	if (list_size(list1) != size2_before || list_size(list2) != size1_before)
	{
		result.passed = false;
		result.message = "交换后大小错误";
		list_free(list1);
		list_free(list2);
		return result;
	}

	// 验证数据完整性
	int front1, front2;
	list_front(list1, &front1);
	list_front(list2, &front2);

	if (front1 != 4 || front2 != 1)
	{
		result.passed = false;
		result.message = "交换后数据错误";
		list_free(list1);
		list_free(list2);
		return result;
	}

	list_free(list1);
	list_free(list2);
	return result;
}

test_result_t test_list_remove(void)
{
	test_result_t result = {"移除操作", true, ""};

	list_handle_t list = list_create(10, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 准备测试数据（包含重复值）
	int values[] = {1, 2, 3, 2, 4, 2, 5};
	for (int i = 0; i < 7; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 自定义谓词函数：移除偶数
	uint16_t removed = list_remove_if(list, is_even, NULL);
	printf("移除偶数后移除的元素数量: %u\n", removed);
	// 输出移除偶数后列表内容
	list_iterator_t it = list_begin(list);
	printf("移除偶数后列表内容: ");
	while (it != NULL)
	{
		printf("%d ", *(int *)it->data);
		it = list_next(it);
	}
	printf("\n");

	if (removed != 4)
	{  // 应该移除2, 2, 4, 2
		result.passed = false;
		result.message = "移除数量错误";
		list_free(list);
		return result;
	}

	// 验证剩余数据
	if (list_size(list) != 3)
	{
		result.passed = false;
		result.message = "移除后大小错误";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_reverse(void)
{
	test_result_t result = {"反转操作", true, ""};

	list_handle_t list = list_create(5, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	int values[] = {1, 2, 3, 4, 5};
	for (int i = 0; i < 5; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 执行反转
	list_reverse(list);

	// 验证反转结果
	int expected[] = {5, 4, 3, 2, 1};
	list_iterator_t it = list_begin(list);
	for (int i = 0; i < 5; i++)
	{
		if (*(int *)it->data != expected[i])
		{
			result.passed = false;
			result.message = "反转后顺序错误";
			list_free(list);
			return result;
		}
		it = list_next(it);
	}

	list_free(list);
	return result;
}

test_result_t test_list_unique(void)
{
	test_result_t result = {"去重操作", true, ""};

	list_handle_t list = list_create(11, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 准备包含重复值的有序数据
	int values[] = {1, 1, 2, 2, 2, 2, 3, 4, 4, 5, 1};
	for (int i = 0; i < 11; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 执行去重
	uint16_t removed = list_unique(list);

	if (removed != 6)
	{  // 应该移除6个重复元素
		result.passed = false;
		result.message = "去重数量错误";
		printf("去重数量错误: %u != 6\n", removed);
		list_free(list);
		return result;
	}

	// 验证去重结果
	if (list_size(list) != 5)
	{
		result.passed = false;
		result.message = "去重后大小错误";
		printf("去重后大小错误: %u != 5\n", list_size(list));
		list_free(list);
		return result;
	}

	// 验证唯一性
	int expected[] = {1, 2, 3, 4, 5};
	list_iterator_t it = list_begin(list);
	for (int i = 0; i < 5; i++)
	{
		if (*(int *)it->data != expected[i])
		{
			result.passed = false;
			result.message = "去重后数据错误";
			printf("去重后数据错误: %d != %d\n", *(int *)it->data, expected[i]);
			list_free(list);
			return result;
		}
		it = list_next(it);
	}

	list_free(list);
	return result;
}

test_result_t test_list_find_if_next(void)
{
	test_result_t result = {"查找匹配节点（支持从指定位置开始）", true, ""};

	list_handle_t list = list_create(20, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 准备测试数据：包含多个匹配值
	int values[] = {1, 2, 3, 2, 4, 2, 5, 2, 6};
	for (int i = 0; i < 9; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 测试1: 查找所有值为2的节点（从头开始）
	int target = 2;
	list_iterator_t it = list_find_if(list, NULL, NULL, &target);
	int found_count = 0;
	int found_positions[10] = {0};  // 记录找到的位置

	while (it != NULL)
	{
		found_positions[found_count] = *(int *)it->data;
		found_count++;
		// 从当前找到的节点之后继续查找
		it = list_find_if(list, it, NULL, &target);
	}

	if (found_count != 4)
	{
		result.passed = false;
		result.message = "应该找到4个值为2的节点";
		list_free(list);
		return result;
	}

	// 验证所有找到的值都是2
	for (int i = 0; i < found_count; i++)
	{
		if (found_positions[i] != 2)
		{
			result.passed = false;
			result.message = "找到的值不正确";
			list_free(list);
			return result;
		}
	}

	// 测试2: 使用谓词函数查找所有偶数（从头开始）
	list_iterator_t it2 = list_find_if(list, NULL, is_even, NULL);
	int even_count = 0;
	while (it2 != NULL)
	{
		even_count++;
		// 从当前找到的节点之后继续查找
		it2 = list_find_if(list, it2, is_even, NULL);
	}

	if (even_count != 6)  // 2, 2, 4, 2, 2, 6
	{
		result.passed = false;
		result.message = "应该找到6个偶数节点";
		list_free(list);
		return result;
	}

	// 测试3: 从NULL开始查找（应该从头开始）
	int target3 = 1;
	list_iterator_t it3 = list_find_if(list, NULL, NULL, &target3);
	if (it3 == NULL || *(int *)it3->data != 1)
	{
		result.passed = false;
		result.message = "从NULL开始查找应该从头开始";
		list_free(list);
		return result;
	}

	// 测试4: 查找不存在的值
	int target4 = 99;
	list_iterator_t it4 = list_find_if(list, NULL, NULL, &target4);
	if (it4 != NULL)
	{
		result.passed = false;
		result.message = "不应该找到不存在的值";
		list_free(list);
		return result;
	}

	// 测试5: 从指定位置开始查找（从中间节点开始）
	// 先找到值为3的节点
	int target5 = 3;
	list_iterator_t it5_start = list_find_if(list, NULL, NULL, &target5);
	if (it5_start == NULL)
	{
		result.passed = false;
		result.message = "应该找到值为3的节点";
		list_free(list);
		return result;
	}

	// 从值为3的节点之后开始查找值为2的节点
	list_iterator_t it5 = list_find_if(list, it5_start, NULL, &target);
	if (it5 == NULL || *(int *)it5->data != 2)
	{
		result.passed = false;
		result.message = "从指定位置开始查找失败";
		list_free(list);
		return result;
	}

	// 测试6: 从最后一个节点开始查找下一个（应该找不到）
	list_iterator_t last = list_end(list);
	list_iterator_t it6 = list_find_if(list, last, NULL, &target);
	if (it6 != NULL)
	{
		result.passed = false;
		result.message = "从最后一个节点开始应该找不到下一个";
		list_free(list);
		return result;
	}

	// 测试7: 从中间位置开始查找，验证查找范围
	// 找到第一个值为2的节点
	list_iterator_t first_2 = list_find_if(list, NULL, NULL, &target);
	if (first_2 == NULL)
	{
		result.passed = false;
		result.message = "应该找到第一个值为2的节点";
		list_free(list);
		return result;
	}

	// 从第一个2之后开始查找，应该找到后续的2
	int count_from_first_2 = 0;
	list_iterator_t it7 = list_find_if(list, first_2, NULL, &target);
	while (it7 != NULL)
	{
		count_from_first_2++;
		it7 = list_find_if(list, it7, NULL, &target);
	}

	if (count_from_first_2 != 3)  // 应该找到后续的3个2
	{
		result.passed = false;
		result.message = "从第一个匹配节点之后应该找到3个匹配节点";
		list_free(list);
		return result;
	}

	list_free(list);
	return result;
}

test_result_t test_list_for_each_if(void)
{
	test_result_t result = {"遍历所有节点并执行回调", true, ""};

	list_handle_t list = list_create(20, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 准备测试数据
	int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	for (int i = 0; i < 10; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 测试1: 使用回调函数收集所有偶数（在回调中过滤）
	collect_data_t collect_data = {0};

	list_for_each_if(list, collect_even, &collect_data);
	if (collect_data.count != 5)  // 2, 4, 6, 8, 10
	{
		result.passed = false;
		result.message = "应该找到5个偶数";
		list_free(list);
		return result;
	}

	// 验证收集的值
	int expected_evens[] = {2, 4, 6, 8, 10};
	for (int i = 0; i < 5; i++)
	{
		if (collect_data.arr[i] != expected_evens[i])
		{
			result.passed = false;
			result.message = "收集的偶数值不正确";
			list_free(list);
			return result;
		}
	}

	// 测试2: 统计所有节点数量
	int all_count = 0;

	list_for_each_if(list, count_all, &all_count);
	if (all_count != 10)
	{
		result.passed = false;
		result.message = "应该处理所有10个节点";
		list_free(list);
		return result;
	}

	// 测试3: 查找特定值（在回调中检查）
	int target = 5;
	int found_count = 0;

	list_for_each_if(list, count_target, &found_count);
	if (found_count != 1)
	{
		result.passed = false;
		result.message = "应该找到1个值为5的节点";
		list_free(list);
		return result;
	}

	// 测试4: 测试空列表
	list_handle_t empty_list = list_create(10, sizeof(int));
	if (empty_list == NULL)
	{
		result.passed = false;
		result.message = "创建空列表失败";
		list_free(list);
		return result;
	}

	int empty_count = 0;
	list_for_each_if(empty_list, count_all, &empty_count);
	if (empty_count != 0)
	{
		result.passed = false;
		result.message = "空列表应该返回0";
		list_free(list);
		list_free(empty_list);
		return result;
	}

	// 测试5: 收集大于5的值（在回调中过滤）
	typedef struct
	{
		int arr[10];
		int count;
		int threshold;
	} filter_data_t;

	filter_data_t filter_data = {0};
	filter_data.threshold = 5;

	list_for_each_if(list, collect_greater, &filter_data);
	if (filter_data.count != 5)  // 6, 7, 8, 9, 10
	{
		result.passed = false;
		result.message = "应该找到5个大于5的节点";
		list_free(list);
		list_free(empty_list);
		return result;
	}

	// 验证收集的值
	int expected_greater[] = {6, 7, 8, 9, 10};
	for (int i = 0; i < 5; i++)
	{
		if (filter_data.arr[i] != expected_greater[i])
		{
			result.passed = false;
			result.message = "收集的大于5的值不正确";
			list_free(list);
			list_free(empty_list);
			return result;
		}
	}

	list_free(list);
	list_free(empty_list);
	return result;
}

test_result_t test_list_recursive_lock(void)
{
	test_result_t result = {"递归锁死锁测试", true, ""};

	list_handle_t list = list_create(20, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	// 准备测试数据
	int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	for (int i = 0; i < 10; i++)
	{
		list_push_back(list, &values[i]);
	}

	// 测试1: 验证 list_remove_if 内部调用 list_erase 不会死锁
	// list_remove_if 会获取锁，然后调用 list_erase，list_erase 也会获取锁
	// 如果使用递归锁，这应该正常工作；如果使用普通锁，会导致死锁
	int target = 5;
	uint16_t removed = list_remove(list, &target);
	if (removed != 1)
	{
		result.passed = false;
		result.message = "应该删除1个值为5的节点";
		list_free(list);
		return result;
	}

	if (list_size(list) != 9)
	{
		result.passed = false;
		result.message = "删除后大小应该为9";
		list_free(list);
		return result;
	}

	// 测试2: 验证 list_unique 内部调用 list_erase 不会死锁
	// 添加重复值
	int dup_values[] = {1, 1, 2, 2, 3};
	for (int i = 0; i < 5; i++)
	{
		list_push_back(list, &dup_values[i]);
	}

	uint16_t unique_removed = list_unique(list);
	if (unique_removed < 2)  // 至少应该删除2个重复值
	{
		result.passed = false;
		result.message = "去重应该删除至少2个重复节点";
		list_free(list);
		return result;
	}

	// 测试3: 验证多层嵌套调用不会死锁
	// 手动模拟嵌套锁的情况
	// 注意：这个测试在无锁模式下也会通过，但在有锁模式下可以验证递归锁
	int test_val = 99;
	list_push_back(list, &test_val);

	// list_remove_if 获取锁 -> list_erase 再次获取锁（递归锁允许）
	uint16_t removed2 = list_remove(list, &test_val);
	if (removed2 != 1)
	{
		result.passed = false;
		result.message = "嵌套锁测试失败";
		list_free(list);
		return result;
	}

	// 如果代码执行到这里，说明递归锁工作正常（没有死锁）
	list_free(list);
	return result;
}

test_result_t test_list_edge_cases(void)
{
	test_result_t result = {"边界情况测试", true, ""};

	// 测试容量为0的列表
	list_handle_t list = list_create(0, sizeof(int));
	if (list != NULL)
	{
		result.passed = false;
		result.message = "容量为0应该创建失败";
		list_free(list);
		return result;
	}

	// 测试元素大小为0的列表
	list = list_create(5, 0);
	if (list != NULL)
	{
		result.passed = false;
		result.message = "元素大小为0应该创建失败";
		list_free(list);
		return result;
	}

	// 测试空指针操作
	if (list_insert(NULL, NULL, NULL) ||
	    list_erase(NULL, NULL) ||
	    list_front(NULL, NULL))
	{
		result.passed = false;
		result.message = "空指针操作应该返回安全值";
		return result;
	}

	return result;
}

test_result_t test_list_splice(void)
{
	test_result_t result = {"拼接操作", true, ""};

	list_handle_t list1 = list_create(10, sizeof(int));
	list_handle_t list2 = list_create(10, sizeof(int));

	if (list1 == NULL || list2 == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		if (list1)
			list_free(list1);
		if (list2)
			list_free(list2);
		return result;
	}

	// 准备测试数据
	int values1[] = {1, 2, 3};
	int values2[] = {10, 20, 30, 40};

	// 填充 list1
	for (int i = 0; i < 3; i++)
	{
		list_push_back(list1, &values1[i]);
	}

	// 填充 list2
	for (int i = 0; i < 4; i++)
	{
		list_push_back(list2, &values2[i]);
	}

	// 测试1: 将 list2 的单个节点移动到 list1 的末尾
	list_iterator_t it2 = list_begin(list2);
	it2 = list_next(it2);                       // 指向值为20的节点
	list_iterator_t it2_next = list_next(it2);  // 指向下一个节点

	if (!list_splice(list1, NULL, list2, it2, it2_next))
	{
		result.passed = false;
		result.message = "拼接单个节点失败";
		list_free(list1);
		list_free(list2);
		return result;
	}

	// 验证 list1: 应该是 [1, 2, 3, 20]
	int expected1[] = {1, 2, 3, 20};
	list_iterator_t it = list_begin(list1);
	for (int i = 0; i < 4; i++)
	{
		if (it == NULL || *(int *)it->data != expected1[i])
		{
			result.passed = false;
			result.message = "拼接后 list1 数据错误";
			printf("拼接后 list1 数据错误: %d != %d\n", *(int *)it->data, expected1[i]);
			list_free(list1);
			list_free(list2);
			return result;
		}
		it = list_next(it);
	}

	// 验证 list2: 应该是 [10, 30, 40]
	int expected2[] = {10, 30, 40};
	it = list_begin(list2);
	for (int i = 0; i < 3; i++)
	{
		if (it == NULL || *(int *)it->data != expected2[i])
		{
			result.passed = false;
			result.message = "拼接后 list2 数据错误";
			list_free(list1);
			list_free(list2);
			return result;
		}
		it = list_next(it);
	}

	// 测试2: 将 list2 的多个节点移动到 list1 的开头
	it2 = list_begin(list2);
	it2_next = list_next(it2);
	it2_next = list_next(it2_next);  // 指向第三个节点（不包含）

	if (!list_splice(list1, list_begin(list1), list2, it2, it2_next))
	{
		result.passed = false;
		result.message = "拼接多个节点失败";
		list_free(list1);
		list_free(list2);
		return result;
	}

	// 验证 list1: 应该是 [10, 30, 1, 2, 3, 20]
	int expected3[] = {10, 30, 1, 2, 3, 20};
	it = list_begin(list1);
	for (int i = 0; i < 6; i++)
	{
		if (it == NULL || *(int *)it->data != expected3[i])
		{
			result.passed = false;
			result.message = "第二次拼接后 list1 数据错误";
			list_free(list1);
			list_free(list2);
			return result;
		}
		it = list_next(it);
	}

	// 验证大小
	if (list_size(list1) != 6 || list_size(list2) != 1)
	{
		result.passed = false;
		result.message = "拼接后大小错误";
		list_free(list1);
		list_free(list2);
		return result;
	}

	list_free(list1);
	list_free(list2);
	return result;
}

test_result_t test_list_merge(void)
{
	test_result_t result = {"合并操作", true, ""};

	list_handle_t list1 = list_create(10, sizeof(int));
	list_handle_t list2 = list_create(10, sizeof(int));

	if (list1 == NULL || list2 == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		if (list1)
			list_free(list1);
		if (list2)
			list_free(list2);
		return result;
	}

	// 准备测试数据
	int values1[] = {1, 3, 5};
	int values2[] = {2, 4, 6};

	// 填充两个列表
	for (int i = 0; i < 3; i++)
	{
		list_push_back(list1, &values1[i]);
		list_push_back(list2, &values2[i]);
	}

	// 使用 list_splice 实现合并：将 list2 的所有节点移动到 list1 的末尾
	list_iterator_t first = list_begin(list2);
	if (first != NULL)
	{
		if (!list_splice(list1, NULL, list2, first, NULL))
		{
			result.passed = false;
			result.message = "合并操作失败";
			list_free(list1);
			list_free(list2);
			return result;
		}
	}

	// 验证 list1 包含所有元素: [1, 3, 5, 2, 4, 6]
	int expected[] = {1, 3, 5, 2, 4, 6};
	list_iterator_t it = list_begin(list1);
	for (int i = 0; i < 6; i++)
	{
		if (it == NULL || *(int *)it->data != expected[i])
		{
			result.passed = false;
			result.message = "合并后 list1 数据错误";
			list_free(list1);
			list_free(list2);
			return result;
		}
		it = list_next(it);
	}

	// 验证 list2 为空
	if (!list_empty(list2) || list_size(list2) != 0)
	{
		result.passed = false;
		result.message = "合并后 list2 应该为空";
		list_free(list1);
		list_free(list2);
		return result;
	}

	// 验证 list1 的大小
	if (list_size(list1) != 6)
	{
		result.passed = false;
		result.message = "合并后 list1 大小错误";
		list_free(list1);
		list_free(list2);
		return result;
	}

	// 测试2: 合并空列表
	list_handle_t list3 = list_create(10, sizeof(int));
	if (list3 == NULL)
	{
		result.passed = false;
		result.message = "创建 list3 失败";
		list_free(list1);
		list_free(list2);
		return result;
	}

	// 将空列表合并到 list1（应该成功但不改变 list1）
	uint16_t size_before = list_size(list1);
	list_iterator_t empty_first = list_begin(list3);
	if (empty_first == NULL)
	{
		// 空列表，合并操作应该成功但不做任何改变
		if (list_size(list1) != size_before)
		{
			result.passed = false;
			result.message = "合并空列表后大小不应该改变";
			list_free(list1);
			list_free(list2);
			list_free(list3);
			return result;
		}
	}

	// 测试3: 将 list1 合并到空列表 list3
	first = list_begin(list1);
	if (first != NULL)
	{
		if (!list_splice(list3, NULL, list1, first, NULL))
		{
			result.passed = false;
			result.message = "合并到空列表失败";
			list_free(list1);
			list_free(list2);
			list_free(list3);
			return result;
		}

		// 验证 list3 包含所有元素
		if (list_size(list3) != 6)
		{
			result.passed = false;
			result.message = "合并到空列表后大小错误";
			list_free(list1);
			list_free(list2);
			list_free(list3);
			return result;
		}
	}

	list_free(list1);
	list_free(list2);
	list_free(list3);
	return result;
}

test_result_t test_list_performance(void)
{
	test_result_t result = {"性能测试", true, ""};

	clock_t start, end;
	double cpu_time_used;

	// 创建大容量列表进行性能测试
	list_handle_t list = list_create(1000, sizeof(int));
	if (list == NULL)
	{
		result.passed = false;
		result.message = "创建失败";
		return result;
	}

	start = clock();

	// 批量插入测试
	for (int i = 0; i < 1000; i++)
	{
		if (!list_push_back(list, &i))
		{
			result.passed = false;
			result.message = "批量插入失败";
			list_free(list);
			return result;
		}
	}

	// 批量删除测试
	for (int i = 0; i < 500; i++)
	{
		if (!list_pop_front(list, NULL))
		{
			result.passed = false;
			result.message = "批量删除失败";
			list_free(list);
			return result;
		}
	}

	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

	printf("性能测试完成，耗时: %.6f 秒\n", cpu_time_used);

	if (cpu_time_used > 1.0)
	{  // 设置性能阈值
		result.passed = false;
		result.message = "性能测试超时";
	}

	list_free(list);
	return result;
}

test_result_t test_list_save_restore(void)
{
	test_result_t result = {"Flash持久化测试", true, ""};
	const char *test_file = "test_list_persist.bin";

	// 创建测试链表并添加数据
	list_handle_t original_list = list_create(20, sizeof(int));
	if (original_list == NULL)
	{
		result.passed = false;
		result.message = "创建原始链表失败";
		return result;
	}

	// 添加测试数据
	int test_data[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
	int test_count = sizeof(test_data) / sizeof(test_data[0]);

	for (int i = 0; i < test_count; i++)
	{
		if (!list_push_back(original_list, &test_data[i]))
		{
			result.passed = false;
			result.message = "添加测试数据失败";
			list_free(original_list);
			return result;
		}
	}

	printf("原始链表大小: %u\n", list_size(original_list));

	// ========== 模拟写入Flash：保存到文件 ==========
	uint32_t serialize_size = list_get_serialize_size(original_list);
	if (serialize_size == 0)
	{
		result.passed = false;
		result.message = "计算序列化大小失败";
		list_free(original_list);
		return result;
	}

	printf("序列化大小: %u 字节\n", serialize_size);

	// 分配缓冲区
	uint8_t *buffer = (uint8_t *)malloc(serialize_size);
	if (buffer == NULL)
	{
		result.passed = false;
		result.message = "分配序列化缓冲区失败";
		list_free(original_list);
		return result;
	}

	// 序列化链表
	uint32_t written = list_serialize(original_list, buffer, serialize_size);
	if (written == 0 || written != serialize_size)
	{
		result.passed = false;
		result.message = "序列化失败";
		free(buffer);
		list_free(original_list);
		return result;
	}

	printf("序列化成功，写入 %u 字节\n", written);

	// 写入文件（模拟写入Flash）
	FILE *fp = fopen(test_file, "wb");
	if (fp == NULL)
	{
		result.passed = false;
		result.message = "打开文件失败";
		free(buffer);
		list_free(original_list);
		return result;
	}

	size_t file_written = fwrite(buffer, 1, written, fp);
	fclose(fp);

	if (file_written != written)
	{
		result.passed = false;
		result.message = "写入文件失败";
		free(buffer);
		list_free(original_list);
		remove(test_file);  // 删除测试文件
		return result;
	}

	printf("数据已保存到文件: %s\n", test_file);
	free(buffer);

	// ========== 模拟断电等待：等待1秒 ==========
	printf("等待1秒（模拟断电）...\n");
	Sleep(1000);  // Windows下等待1秒
	printf("恢复上电，开始读取数据...\n");

	// ========== 模拟从Flash读取：从文件读取 ==========
	fp = fopen(test_file, "rb");
	if (fp == NULL)
	{
		result.passed = false;
		result.message = "打开文件读取失败";
		list_free(original_list);
		remove(test_file);
		return result;
	}

	// 获取文件大小
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (file_size != (long)serialize_size)
	{
		result.passed = false;
		result.message = "文件大小不匹配";
		fclose(fp);
		list_free(original_list);
		remove(test_file);
		return result;
	}

	// 读取文件内容
	uint8_t *read_buffer = (uint8_t *)malloc(file_size);
	if (read_buffer == NULL)
	{
		result.passed = false;
		result.message = "分配读取缓冲区失败";
		fclose(fp);
		list_free(original_list);
		remove(test_file);
		return result;
	}

	size_t file_read = fread(read_buffer, 1, file_size, fp);
	fclose(fp);

	if (file_read != (size_t)file_size)
	{
		result.passed = false;
		result.message = "读取文件失败";
		free(read_buffer);
		list_free(original_list);
		remove(test_file);
		return result;
	}

	printf("从文件读取 %zu 字节\n", file_read);

	// ========== 反序列化恢复链表 ==========
	// 测试容量升级：使用更大的容量恢复（向后兼容）
	list_handle_t restored_list = list_create(original_list->capacity + 10, original_list->element_size);
	if (restored_list == NULL)
	{
		result.passed = false;
		result.message = "创建恢复链表失败";
		free(read_buffer);
		list_free(original_list);
		remove(test_file);
		return result;
	}

	// 反序列化（使用更大的容量，应该成功）
	if (!list_deserialize(restored_list, read_buffer, file_size))
	{
		result.passed = false;
		result.message = "反序列化失败";
		free(read_buffer);
		list_free(original_list);
		list_free(restored_list);
		remove(test_file);
		return result;
	}

	printf("反序列化成功，恢复链表大小: %u，容量: %u (原始容量: %u)\n",
	       list_size(restored_list), list_capacity(restored_list), original_list->capacity);

	// ========== 验证数据正确性 ==========
	// 检查大小
	if (list_size(restored_list) != list_size(original_list))
	{
		result.passed = false;
		result.message = "恢复后链表大小不匹配";
		list_free(original_list);
		list_free(restored_list);
		remove(test_file);
		return result;
	}

	if (list_size(restored_list) != (uint16_t)test_count)
	{
		result.passed = false;
		result.message = "恢复后链表大小与测试数据不匹配";
		list_free(original_list);
		list_free(restored_list);
		remove(test_file);
		return result;
	}

	// 检查数据内容
	list_iterator_t orig_it = list_begin(original_list);
	list_iterator_t rest_it = list_begin(restored_list);

	for (int i = 0; i < test_count; i++)
	{
		if (orig_it == NULL || rest_it == NULL)
		{
			result.passed = false;
			result.message = "链表迭代器为空";
			list_free(original_list);
			list_free(restored_list);
			remove(test_file);
			return result;
		}

		int orig_val = *(int *)orig_it->data;
		int rest_val = *(int *)rest_it->data;

		if (orig_val != rest_val)
		{
			result.passed = false;
			result.message = "恢复后数据不匹配";
			printf("位置 %d: 原始=%d, 恢复=%d\n", i, orig_val, rest_val);
			list_free(original_list);
			list_free(restored_list);
			remove(test_file);
			return result;
		}

		if (orig_val != test_data[i])
		{
			result.passed = false;
			result.message = "原始数据与测试数据不匹配";
			list_free(original_list);
			list_free(restored_list);
			remove(test_file);
			return result;
		}

		orig_it = list_next(orig_it);
		rest_it = list_next(rest_it);
	}

	// ========== 验证链表结构完整性 ==========
	// 验证正向遍历
	rest_it = list_begin(restored_list);
	int count = 0;
	while (rest_it != NULL)
	{
		count++;
		rest_it = list_next(rest_it);
	}
	if (count != test_count)
	{
		result.passed = false;
		result.message = "正向遍历节点数量不匹配";
		list_free(original_list);
		list_free(restored_list);
		remove(test_file);
		return result;
	}

	// 验证反向遍历
	rest_it = list_end(restored_list);
	count = 0;
	while (rest_it != NULL)
	{
		count++;
		rest_it = list_prev(rest_it);
	}
	if (count != test_count)
	{
		result.passed = false;
		result.message = "反向遍历节点数量不匹配";
		list_free(original_list);
		list_free(restored_list);
		remove(test_file);
		return result;
	}

	// 验证容量升级：恢复后的链表容量应该大于原始链表
	if (list_capacity(restored_list) < original_list->capacity)
	{
		result.passed = false;
		result.message = "容量升级验证失败";
		list_free(original_list);
		list_free(restored_list);
		remove(test_file);
		return result;
	}

	// 验证容量缩小会失败（需要重新读取文件，因为read_buffer可能已被使用）
	fp = fopen(test_file, "rb");
	if (fp != NULL)
	{
		uint8_t *test_buffer = (uint8_t *)malloc(file_size);
		if (test_buffer != NULL)
		{
			fread(test_buffer, 1, file_size, fp);
			fclose(fp);

			// 尝试用更小的容量恢复，应该失败
			list_handle_t small_list = list_create(original_list->capacity - 5, original_list->element_size);
			if (small_list != NULL)
			{
				if (list_deserialize(small_list, test_buffer, file_size))
				{
					result.passed = false;
					result.message = "容量缩小应该失败但却成功了";
					list_free(original_list);
					list_free(restored_list);
					list_free(small_list);
					free(test_buffer);
					remove(test_file);
					return result;
				}
				list_free(small_list);
			}
			free(test_buffer);
		}
		else
		{
			fclose(fp);
		}
	}

	// 释放读取缓冲区
	free(read_buffer);

	// 验证头尾节点数据
	int front_val, back_val;
	if (!list_front(restored_list, &front_val) || front_val != test_data[0])
	{
		result.passed = false;
		result.message = "头部数据不正确";
		list_free(original_list);
		list_free(restored_list);
		remove(test_file);
		return result;
	}

	if (!list_back(restored_list, &back_val) || back_val != test_data[test_count - 1])
	{
		result.passed = false;
		result.message = "尾部数据不正确";
		list_free(original_list);
		list_free(restored_list);
		remove(test_file);
		return result;
	}

	printf("数据验证通过！\n");
	printf("  原始数据: ");
	for (int i = 0; i < test_count; i++)
	{
		printf("%d ", test_data[i]);
	}
	printf("\n  恢复数据: ");
	rest_it = list_begin(restored_list);
	while (rest_it != NULL)
	{
		printf("%d ", *(int *)rest_it->data);
		rest_it = list_next(rest_it);
	}
	printf("\n");

	// 清理
	list_free(original_list);
	list_free(restored_list);
	remove(test_file);  // 删除测试文件
	printf("测试文件已清理: %s\n", test_file);

	return result;
}

// ========================= 主测试函数 =========================

void run_all_tests(void)
{
	printf("开始运行链表单元测试...\n");
	printf("==============================\n");

	tests_passed = 0;
	tests_failed = 0;

	// 运行所有测试用例
	print_test_result(test_list_creation());
	print_test_result(test_list_static_creation());
	print_test_result(test_list_basic_operations());
	print_test_result(test_list_push_pop());
	print_test_result(test_list_insert_erase());
	print_test_result(test_list_front_back());
	print_test_result(test_list_clear());
	print_test_result(test_list_replace());
	print_test_result(test_list_swap());
	print_test_result(test_list_remove());
	print_test_result(test_list_reverse());
	print_test_result(test_list_unique());
	print_test_result(test_list_find_if_next());
	print_test_result(test_list_for_each_if());
	print_test_result(test_list_recursive_lock());
	print_test_result(test_list_splice());
	print_test_result(test_list_merge());
	print_test_result(test_list_edge_cases());
	print_test_result(test_list_performance());
	print_test_result(test_list_save_restore());

	printf("==============================\n");
	printf("测试完成！\n");
	printf("通过: %d, 失败: %d, 总计: %d\n",
	       tests_passed, tests_failed, tests_passed + tests_failed);

	if (tests_failed == 0)
	{
		printf("* 所有测试用例全部通过！\n");
	}
	else
	{
		printf("$ 有测试用例失败，请检查实现代码\n");
	}
}