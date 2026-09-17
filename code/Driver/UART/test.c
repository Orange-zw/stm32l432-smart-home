#include "stm32f10x.h"
#include "usart_factory.h"
#include "stdio.h"
#include "delay.h"

// 接收完成回调函数
void my_rx_callback(uint8_t *data, uint16_t length)
{
    printf("Received %d bytes: ", length);
    for (uint16_t i = 0; i < length; i++)
    {
        printf("%c", data[i]);
    }
    printf("\n");
}

// 发送完成回调函数
void my_tx_callback(void)
{
    printf("Transmission completed!\n");
}

int main(void)
{
    // 系统初始化
    SystemInit();
    delay_init();

    printf("USART Factory Test Start...\n");

    // 使用工厂创建USART实例
    USART_handle_t usart1 = USARTFactory_Create(USART_INSTANCE_1, 115200);
    USART_handle_t usart2 = USARTFactory_Create(USART_INSTANCE_2, 9600);

    if (usart1 == NULL || usart2 == NULL)
    {
        printf("USART creation failed!\n");
        while (1)
            ;
    }

    // 获取操作接口
    USART_Operations *ops = USART_GetOperations();

    // 初始化USART
    if (!ops->init(usart1))
    {
        printf("USART1 initialization failed!\n");
        while (1)
            ;
    }

    if (!ops->init(usart2))
    {
        printf("USART2 initialization failed!\n");
        while (1)
            ;
    }

    printf("USART instances created and initialized\n");

    // 设置回调函数
    ops->set_rx_idle_callback(usart1, my_rx_callback);
    ops->set_tx_callback(usart1, my_tx_callback);

    printf("Callbacks set\n");

    // 测试数据
    uint8_t test_data[] = "Hello, USART!";
    uint8_t rx_buffer[256];

    while (1)
    {
        // 发送数据
        uint16_t sent = ops->send(usart1, test_data, strlen((char *)test_data));
        printf("Sent %d bytes\n", sent);

        // 检查接收数据
        if (ops->is_rx_ready(usart1))
        {
            uint16_t received = ops->receive(usart1, rx_buffer, sizeof(rx_buffer));
            if (received > 0)
            {
                printf("Manual receive: ");
                for (uint16_t i = 0; i < received; i++)
                {
                    printf("%c", rx_buffer[i]);
                }
                printf("\n");
            }
        }

        delay_ms(1000);
    }

    // 清理资源
    USARTFactory_DestroyAll();

    return 0;
}