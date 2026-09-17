#include "JW01.h"
static volatile unsigned short JW01_CO2 = 0;
static volatile unsigned short JW01_TVOC_RAW = 0;
static volatile unsigned short JW01_CH2O_RAW = 0;
static unsigned char jw01_buf[9];
static unsigned char jw01_state = 0;
static void JW01_RxHandler(unsigned char b)
{
#if JW01_TYPE==JW01_3IN1
    switch(jw01_state)
    {
        case 0:
            if(b==0x2C){jw01_buf[0]=b;jw01_state=1;} else {jw01_state=0;}
            break;
        case 1:
            if(b==0xE4){jw01_buf[1]=b;jw01_state=2;} else {jw01_state=(b==0x2C)?1:0;}
            break;
        case 2:
            jw01_buf[2]=b;jw01_state=3;
            break;
        case 3:
            jw01_buf[3]=b;jw01_state=4;
            break;
        case 4:
            jw01_buf[4]=b;jw01_state=5;
            break;
        case 5:
            jw01_buf[5]=b;jw01_state=6;
            break;
        case 6:
            jw01_buf[6]=b;jw01_state=7;
            break;
        case 7:
            jw01_buf[7]=b;jw01_state=8;
            break;
        case 8:
        {
            jw01_buf[8]=b;
            unsigned int s = jw01_buf[0]+jw01_buf[1]+jw01_buf[2]+jw01_buf[3]+jw01_buf[4]+jw01_buf[5]+jw01_buf[6]+jw01_buf[7];
            unsigned char sum = (unsigned char)(s & 0xFF);
            if(sum==jw01_buf[8])
            {
                JW01_TVOC_RAW = ((unsigned short)jw01_buf[2]<<8)|jw01_buf[3];
                JW01_CH2O_RAW = ((unsigned short)jw01_buf[4]<<8)|jw01_buf[5];
                JW01_CO2 = ((unsigned short)jw01_buf[6]<<8)|jw01_buf[7];
            }
            jw01_state=0;
        }
            break;
        default:
            jw01_state=0;
            break;
    }
#else
    switch(jw01_state)
    {
        case 0:
            if(b==0x2C){jw01_buf[0]=b;jw01_state=1;} else {jw01_state=0;}
            break;
        case 1:
            jw01_buf[1]=b;jw01_state=2;
            break;
        case 2:
            jw01_buf[2]=b;jw01_state=3;
            break;
        case 3:
            jw01_buf[3]=b;jw01_state=4;
            break;
        case 4:
            jw01_buf[4]=b;jw01_state=5;
            break;
        case 5:
        {
            jw01_buf[5]=b;
            unsigned int s = jw01_buf[0]+jw01_buf[1]+jw01_buf[2]+jw01_buf[3]+jw01_buf[4];
            unsigned char sum = (unsigned char)(s & 0xFF);
            if(sum==jw01_buf[5])
            {
                JW01_CO2 = ((unsigned short)jw01_buf[1]<<8)|jw01_buf[2];
            }
            jw01_state=0;
        }
            break;
        default:
            jw01_state=0;
            break;
    }
#endif
}

unsigned short JW01_GetCO2(void)
{
    return JW01_CO2;
}

float JW01_GetTVOC(void)
{
    return (float)JW01_TVOC_RAW* 0.001f;
}

float JW01_GetCH2O(void)
{
    return (float)JW01_CH2O_RAW * 0.001f;
}

void JW01_Init(unsigned int bound)
{
#if JW01_USART1
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
#elif JW01_USART2
    GPIO_InitTypeDef gpio_initstruct;
    USART_InitTypeDef usart_initstruct;
    NVIC_InitTypeDef nvic_initstruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    gpio_initstruct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_initstruct.GPIO_Pin = GPIO_Pin_2;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_initstruct);

    gpio_initstruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_initstruct.GPIO_Pin = GPIO_Pin_3;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_initstruct);

    usart_initstruct.USART_BaudRate = bound;
    usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart_initstruct.USART_Parity = USART_Parity_No;
    usart_initstruct.USART_StopBits = USART_StopBits_1;
    usart_initstruct.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2, &usart_initstruct);

    USART_Cmd(USART2, ENABLE);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    nvic_initstruct.NVIC_IRQChannel = USART2_IRQn;
    nvic_initstruct.NVIC_IRQChannelCmd = ENABLE;
    nvic_initstruct.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_initstruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&nvic_initstruct);
#elif JW01_USART3
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    USART_DeInit(USART3);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART3, &USART_InitStructure);
    USART_Cmd(USART3, ENABLE);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
}

#if JW01_USART1
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(JW01_USART, USART_IT_RXNE) != RESET)
    {
        uint16_t d = USART_ReceiveData(JW01_USART);
        JW01_RxHandler((unsigned char)d);
    }
    USART_ClearFlag(JW01_USART, USART_FLAG_RXNE);
}
#elif JW01_USART2
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(JW01_USART, USART_IT_RXNE) != RESET)
    {
        uint16_t d = USART_ReceiveData(JW01_USART);
        JW01_RxHandler((unsigned char)d);
    }
    USART_ClearFlag(JW01_USART, USART_FLAG_RXNE);
}
#elif JW01_USART3
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(JW01_USART, USART_IT_RXNE) != RESET)
    {
        uint16_t d = USART_ReceiveData(JW01_USART);
        JW01_RxHandler((unsigned char)d);
    }
    USART_ClearFlag(JW01_USART, USART_FLAG_RXNE);
}
#endif
