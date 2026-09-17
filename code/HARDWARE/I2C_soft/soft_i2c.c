#include "soft_i2c.h"

// 私有函数声明
static void I2C_Delay(I2C_HandleTypeDef *hi2c);
static void SCL_High(I2C_HandleTypeDef *hi2c);
static void SCL_Low(I2C_HandleTypeDef *hi2c);
static void SDA_High(I2C_HandleTypeDef *hi2c);
static void SDA_Low(I2C_HandleTypeDef *hi2c);
static uint8_t SDA_Read(I2C_HandleTypeDef *hi2c);

static void I2C_Delay(I2C_HandleTypeDef *hi2c)
{
    delay_us(hi2c->delay_us);
}

static void SCL_High(I2C_HandleTypeDef *hi2c)
{
    GPIO_SetBits(hi2c->scl_port, hi2c->scl_pin);
}

static void SCL_Low(I2C_HandleTypeDef *hi2c)
{
    GPIO_ResetBits(hi2c->scl_port, hi2c->scl_pin);
}

static void SDA_High(I2C_HandleTypeDef *hi2c)
{
    GPIO_SetBits(hi2c->sda_port, hi2c->sda_pin);
}

static void SDA_Low(I2C_HandleTypeDef *hi2c)
{
    GPIO_ResetBits(hi2c->sda_port, hi2c->sda_pin);
}

static uint8_t SDA_Read(I2C_HandleTypeDef *hi2c)
{
    return GPIO_ReadInputDataBit(hi2c->sda_port, hi2c->sda_pin);
}

I2C_Status SoftI2C_Init(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    if (hi2c->initialized)
        return I2C_OK;

    // 设置默认延时
    if (hi2c->delay_us == 0)
    {
        hi2c->delay_us = 5;
    }

    // 使能时钟
    if (hi2c->scl_port == GPIOA)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    }
    else if (hi2c->scl_port == GPIOB)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    }
    else if (hi2c->scl_port == GPIOC)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    }

    if (hi2c->sda_port == GPIOA)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    }
    else if (hi2c->sda_port == GPIOB)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    }
    else if (hi2c->sda_port == GPIOC)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    }

    // 配置SCL引脚
    GPIO_InitStructure.GPIO_Pin = hi2c->scl_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(hi2c->scl_port, &GPIO_InitStructure);

    // 配置SDA引脚
    GPIO_InitStructure.GPIO_Pin = hi2c->sda_pin;
    GPIO_Init(hi2c->sda_port, &GPIO_InitStructure);

    // 初始化为高电平
    SCL_High(hi2c);
    SDA_High(hi2c);

    hi2c->initialized = true;
    return I2C_OK;
}

I2C_Status SoftI2C_Deinit(I2C_HandleTypeDef *hi2c)
{
    if (!hi2c->initialized)
        return I2C_OK;

    hi2c->initialized = false;
    return I2C_OK;
}

I2C_Status SoftI2C_Start(I2C_HandleTypeDef *hi2c)
{
    SDA_High(hi2c);
    SCL_High(hi2c);
    I2C_Delay(hi2c);
    SDA_Low(hi2c);
    I2C_Delay(hi2c);
    SCL_Low(hi2c);
    I2C_Delay(hi2c);
    return I2C_OK;
}

I2C_Status SoftI2C_Stop(I2C_HandleTypeDef *hi2c)
{
    SDA_Low(hi2c);
    SCL_Low(hi2c);
    I2C_Delay(hi2c);
    SCL_High(hi2c);
    I2C_Delay(hi2c);
    SDA_High(hi2c);
    I2C_Delay(hi2c);
    return I2C_OK;
}

I2C_Status SoftI2C_WaitAck(I2C_HandleTypeDef *hi2c)
{
    uint8_t time = 0;
    SDA_High(hi2c);
    I2C_Delay(hi2c);
    SCL_High(hi2c);
    I2C_Delay(hi2c);

    while (SDA_Read(hi2c))
    {
        time++;
        if (time > 250)
        {
            SoftI2C_Stop(hi2c);
            return I2C_NACK;
        }
    }
    SCL_Low(hi2c);
    I2C_Delay(hi2c);
    return I2C_OK;
}

I2C_Status SoftI2C_SendAck(I2C_HandleTypeDef *hi2c)
{
    SDA_Low(hi2c);
    I2C_Delay(hi2c);
    SCL_High(hi2c);
    I2C_Delay(hi2c);
    SCL_Low(hi2c);
    I2C_Delay(hi2c);
    return I2C_OK;
}

I2C_Status SoftI2C_SendNack(I2C_HandleTypeDef *hi2c)
{
    SDA_High(hi2c);
    I2C_Delay(hi2c);
    SCL_High(hi2c);
    I2C_Delay(hi2c);
    SCL_Low(hi2c);
    I2C_Delay(hi2c);
    return I2C_OK;
}

I2C_Status SoftI2C_SendByte(I2C_HandleTypeDef *hi2c, uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            SDA_High(hi2c);
        }
        else
        {
            SDA_Low(hi2c);
        }
        I2C_Delay(hi2c);
        SCL_High(hi2c);
        I2C_Delay(hi2c);
        SCL_Low(hi2c);
        I2C_Delay(hi2c);
        data <<= 1;
    }
    return I2C_OK;
}

uint8_t SoftI2C_ReadByte(I2C_HandleTypeDef *hi2c)
{
    uint8_t i, data = 0;
    SDA_High(hi2c);
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        SCL_High(hi2c);
        I2C_Delay(hi2c);
        if (SDA_Read(hi2c))
        {
            data |= 0x01;
        }
        SCL_Low(hi2c);
        I2C_Delay(hi2c);
    }
    return data;
}

I2C_Status SoftI2C_WriteByte(I2C_HandleTypeDef *hi2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    I2C_Status status;

    status = SoftI2C_Start(hi2c);
    if (status != I2C_OK)
        return status;

    status = SoftI2C_SendByte(hi2c, dev_addr << 1);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_WaitAck(hi2c);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_SendByte(hi2c, reg_addr);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_WaitAck(hi2c);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_SendByte(hi2c, data);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_WaitAck(hi2c);
    if (status != I2C_OK)
        goto error;

    SoftI2C_Stop(hi2c);
    return I2C_OK;

error:
    SoftI2C_Stop(hi2c);
    return status;
}

I2C_Status SoftI2C_ReadBytes(I2C_HandleTypeDef *hi2c, uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    I2C_Status status;
    uint16_t i;

    status = SoftI2C_Start(hi2c);
    if (status != I2C_OK)
        return status;

    status = SoftI2C_SendByte(hi2c, dev_addr << 1);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_WaitAck(hi2c);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_SendByte(hi2c, reg_addr);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_WaitAck(hi2c);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_Start(hi2c);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_SendByte(hi2c, (dev_addr << 1) | 0x01);
    if (status != I2C_OK)
        goto error;

    status = SoftI2C_WaitAck(hi2c);
    if (status != I2C_OK)
        goto error;

    for (i = 0; i < len; i++)
    {
        data[i] = SoftI2C_ReadByte(hi2c);
        if (i == len - 1)
        {
            SoftI2C_SendNack(hi2c);
        }
        else
        {
            SoftI2C_SendAck(hi2c);
        }
    }

    SoftI2C_Stop(hi2c);
    return I2C_OK;

error:
    SoftI2C_Stop(hi2c);
    return status;
}

I2C_Status SoftI2C_ProbeDevice(I2C_HandleTypeDef *hi2c, uint8_t dev_addr)
{
    I2C_Status status;

    status = SoftI2C_Start(hi2c);
    if (status != I2C_OK)
        return status;

    status = SoftI2C_SendByte(hi2c, dev_addr << 1);
    if (status != I2C_OK)
    {
        SoftI2C_Stop(hi2c);
        return status;
    }

    status = SoftI2C_WaitAck(hi2c);
    SoftI2C_Stop(hi2c);

    return status;
}