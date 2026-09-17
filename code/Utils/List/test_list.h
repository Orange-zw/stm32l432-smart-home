#ifndef TEST_LIST_H
#define TEST_LIST_H

#include "embedded_list.h"
#include <stdio.h>
#include <string.h>

// 测试结果结构体
typedef struct
{
    const char *test_name;
    bool passed;
    const char *message;
} test_result_t;

// 测试函数声明
void run_all_tests(void);

// 具体测试用例
test_result_t test_list_creation(void);
test_result_t test_list_static_creation(void);
test_result_t test_list_basic_operations(void);
test_result_t test_list_push_pop(void);
test_result_t test_list_insert_erase(void);
test_result_t test_list_front_back(void);
test_result_t test_list_clear(void);
test_result_t test_list_replace(void);
test_result_t test_list_swap(void);
test_result_t test_list_merge(void);
test_result_t test_list_splice(void);
test_result_t test_list_remove(void);
test_result_t test_list_reverse(void);
test_result_t test_list_unique(void);
test_result_t test_list_find_if_next(void);
test_result_t test_list_for_each_if(void);
test_result_t test_list_recursive_lock(void);
test_result_t test_list_edge_cases(void);
test_result_t test_list_performance(void);
test_result_t test_list_save_restore(void);

// 测试辅助函数
void print_test_result(test_result_t result);
bool compare_int(const void *a, const void *b);
bool is_even(const void *list_data, const void *predicate_data);
bool is_positive(const void *data);

#endif