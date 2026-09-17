#include "test_list.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

int main(void)
{
#ifdef _WIN32
    // 设置控制台代码页为UTF-8，解决中文乱码问题
    SetConsoleOutputCP(65001);  // UTF-8代码页
    SetConsoleCP(65001);        // 同时设置输入代码页
#endif

    printf("链表单元测试程序\n");
    printf("================\n\n");

    run_all_tests();

    return 0;
}