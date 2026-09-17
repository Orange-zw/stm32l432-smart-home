#include <stdio.h>
#include "ring_buf.h"

int main()
{
    // 1. 创建大小为5的环形缓冲区
    ringbuf_t *buf = ringbuf_create(5);
    if (buf == NULL)
    {
        printf("错误：缓冲区创建失败！\n");
        return -1;
    }
    printf("缓冲区创建成功，容量：%u\n", buf->size);

    uint8_t write_data[] = {1, 2, 3, 4, 5, 6}; // 测试数据（超过缓冲区容量）
    uint8_t read_data[10] = {0};               // 存储读取的数据
    uint16_t bytes_read;

    // 2. 写入6个字节（缓冲区大小为5，会覆盖第一个数据）
    ringbuf_write(buf, write_data, 6);
    printf("已写入数据：1, 2, 3, 4, 5, 6（注意：6覆盖了1）\n");
    printf("当前缓冲区有效数据长度：%u\n", ringbuf_get_len(buf));

    // 3. 读取3个字节
    bytes_read = ringbuf_read(buf, read_data, 3);
    printf("已读取 %u 个字节：", bytes_read);
    printf("当前缓冲区有效数据长度：%u\n", ringbuf_get_len(buf));
    for (int i = 0; i < bytes_read; i++)
    {
        printf("%d ", read_data[i]);
    }
    printf("\n");

    // 4. 再写入2个字节
    uint8_t write_data2[] = {7, 8};
    ringbuf_write(buf, write_data2, 2);
    printf("已写入数据：7, 8\n");
    printf("当前缓冲区有效数据长度：%u\n", ringbuf_get_len(buf));

    // 5. 尝试读取10个字节（实际最多可读4个）
    bytes_read = ringbuf_read(buf, read_data, 10);
    printf("已读取 %u 个字节：", bytes_read);
    printf("当前缓冲区有效数据长度：%u\n", ringbuf_get_len(buf));
    for (int i = 0; i < bytes_read; i++)
    {
        printf("%d ", read_data[i]);
    }
    printf("\n");

    // 6. 清理资源
    ringbuf_free(buf);
    printf("缓冲区已销毁\n");
    return 0;
}