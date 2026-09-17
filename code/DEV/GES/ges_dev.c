#include "ges_dev.h"
#include "../../Driver/SOFT_I2C/soft_i2c_master.h"
#include "delay.h"

/* ======== 默认连线（仅用于容错，不建议依赖） ======== */
#define GES_I2C_DEV_ADDR (0x73U) /* PAJ7620U2 7bit 地址 */
#define GES_I2C_SCL_PORT (GPIOB)
#define GES_I2C_SCL_PIN (GPIO_Pin_14)
#define GES_I2C_SDA_PORT (GPIOB)
#define GES_I2C_SDA_PIN (GPIO_Pin_15)
#define GES_I2C_FREQ (SOFT_I2C_100KHZ)
#define GES_MAX_DEVICES (2U)

/* ======== PAJ7620U2 核心寄存器 ======== */
#define PAJ_REG_BANK_SEL (0xEFU)
#define PAJ_BANK0 (0x00U)
#define PAJ_BANK1 (0x01U)
#define PAJ_REG_GES_INT_FLAG1 (0x43U)
#define PAJ_REG_PART_ID_LOW (0x00U)

typedef struct
{
	uint8_t               in_use;
	GES_dev_t             dev;
	soft_i2c_master_bus_t i2c_bus;
} GES_Context_t;

static GES_Context_t g_ges_pool[GES_MAX_DEVICES];

/* 上电初始化序列（来自官方推荐流程） */
static const uint8_t g_init_array[][2] = {
    {0xEF, 0x00},
    {0x37, 0x07},
    {0x38, 0x17},
    {0x39, 0x06},
    {0x41, 0x00},
    {0x42, 0x00},
    {0x46, 0x2D},
    {0x47, 0x0F},
    {0x48, 0x3C},
    {0x49, 0x00},
    {0x4A, 0x1E},
    {0x4C, 0x20},
    {0x51, 0x10},
    {0x5E, 0x10},
    {0x60, 0x27},
    {0x80, 0x42},
    {0x81, 0x44},
    {0x82, 0x04},
    {0x8B, 0x01},
    {0x90, 0x06},
    {0x95, 0x0A},
    {0x96, 0x0C},
    {0x97, 0x05},
    {0x9A, 0x14},
    {0x9C, 0x3F},
    {0xA5, 0x19},
    {0xCC, 0x19},
    {0xCD, 0x0B},
    {0xCE, 0x13},
    {0xCF, 0x64},
    {0xD0, 0x21},
    {0xEF, 0x01},
    {0x02, 0x0F},
    {0x03, 0x10},
    {0x04, 0x02},
    {0x25, 0x01},
    {0x27, 0x39},
    {0x28, 0x7F},
    {0x29, 0x08},
    {0x3E, 0xFF},
    {0x5E, 0x3D},
    {0x65, 0x96},
    {0x67, 0x97},
    {0x69, 0xCD},
    {0x6A, 0x01},
    {0x6D, 0x2C},
    {0x6E, 0x01},
    {0x72, 0x01},
    {0x73, 0x35},
    {0x74, 0x00},
    {0x77, 0x01},
};

/* 手势模式初始化序列 */
static const uint8_t g_gesture_array[][2] = {
    {0xEF, 0x00},
    {0x41, 0x00},
    {0x42, 0x00},
    {0xEF, 0x00},
    {0x48, 0x3C},
    {0x49, 0x00},
    {0x51, 0x10},
    {0x83, 0x20},
    {0x9F, 0xF9},
    {0xEF, 0x01},
    {0x01, 0x1E},
    {0x02, 0x0F},
    {0x03, 0x10},
    {0x04, 0x02},
    {0x41, 0x40},
    {0x43, 0x30},
    {0x65, 0x96},
    {0x66, 0x00},
    {0x67, 0x97},
    {0x68, 0x01},
    {0x69, 0xCD},
    {0x6A, 0x01},
    {0x6B, 0xB0},
    {0x6C, 0x04},
    {0x6D, 0x2C},
    {0x6E, 0x01},
    {0x74, 0x00},
    {0xEF, 0x00},
    {0x41, 0xFF},
    {0x42, 0x01},
};

static GES_Context_t *GES_GetContext(GES_handle_t dev);
static uint8_t        GES_WriteReg(GES_Context_t *ctx, uint8_t reg, uint8_t val);
static uint8_t        GES_ReadReg(GES_Context_t *ctx, uint8_t reg, uint8_t *val);
static uint8_t        GES_ReadRegs(GES_Context_t *ctx, uint8_t reg, uint8_t *buf, uint16_t len);
static uint8_t        GES_WritePairs(GES_Context_t *ctx, const uint8_t (*cfg)[2], uint16_t size);
static uint8_t        GES_InitDevice(GES_Context_t *ctx);

static GES_Context_t *GES_GetContext(GES_handle_t dev)
{
	uint8_t i;

	if (dev == 0)
	{
		return 0;
	}

	for (i = 0U; i < GES_MAX_DEVICES; i++)
	{
		if (g_ges_pool[i].in_use != 0U && &g_ges_pool[i].dev == dev)
		{
			return &g_ges_pool[i];
		}
	}
	return 0;
}

static uint8_t GES_WriteReg(GES_Context_t *ctx, uint8_t reg, uint8_t val)
{
	uint8_t        tx[2];
	soft_i2c_err_t ret;

	tx[0] = reg;
	tx[1] = val;
	ret = soft_i2c_master_write(ctx->i2c_bus, GES_I2C_DEV_ADDR, tx, 2U);
	return (ret == SOFT_I2C_OK) ? 1U : 0U;
}

static uint8_t GES_ReadReg(GES_Context_t *ctx, uint8_t reg, uint8_t *val)
{
	soft_i2c_err_t ret;
	ret = soft_i2c_master_write_read(ctx->i2c_bus, GES_I2C_DEV_ADDR, &reg, 1U, val, 1U);
	return (ret == SOFT_I2C_OK) ? 1U : 0U;
}

static uint8_t GES_ReadRegs(GES_Context_t *ctx, uint8_t reg, uint8_t *buf, uint16_t len)
{
	soft_i2c_err_t ret;
	ret = soft_i2c_master_write_read(ctx->i2c_bus, GES_I2C_DEV_ADDR, &reg, 1U, buf, len);
	return (ret == SOFT_I2C_OK) ? 1U : 0U;
}

static uint8_t GES_WritePairs(GES_Context_t *ctx, const uint8_t (*cfg)[2], uint16_t size)
{
	uint16_t i;
	for (i = 0; i < size; i++)
	{
		if (!GES_WriteReg(ctx, cfg[i][0], cfg[i][1]))
		{
			return 0U;
		}
	}
	return 1U;
}

static uint8_t GES_InitDevice(GES_Context_t *ctx)
{
	soft_i2c_master_config_t i2c_cfg;
	uint8_t                  id_low = 0U;
	uint16_t                 init_size = (uint16_t)(sizeof(g_init_array) / sizeof(g_init_array[0]));
	uint16_t                 ges_size = (uint16_t)(sizeof(g_gesture_array) / sizeof(g_gesture_array[0]));

	if (ctx->dev.is_initialized != 0U && ctx->dev.state == GES_STATE_READY)
	{
		return 1U;
	}

	i2c_cfg.scl_port = ctx->dev.scl_port;
	i2c_cfg.scl_pin = ctx->dev.scl_pin;
	i2c_cfg.sda_port = ctx->dev.sda_port;
	i2c_cfg.sda_pin = ctx->dev.sda_pin;
	i2c_cfg.freq = GES_I2C_FREQ;

	if (soft_i2c_master_new(&i2c_cfg, &ctx->i2c_bus) != SOFT_I2C_OK)
	{
		ctx->dev.state = GES_STATE_ERROR;
		return 0U;
	}

	/* 按手册要求，留出上电稳定时间后再访问寄存器 */
	delay_ms(5);

	if (!GES_WriteReg(ctx, PAJ_REG_BANK_SEL, PAJ_BANK0))
	{
		ctx->dev.state = GES_STATE_ERROR;
		return 0U;
	}

	if (!GES_ReadReg(ctx, PAJ_REG_PART_ID_LOW, &id_low))
	{
		ctx->dev.state = GES_STATE_ERROR;
		return 0U;
	}

	if (id_low != 0x20U)
	{
		ctx->dev.state = GES_STATE_ERROR;
		return 0U;
	}

	if (!GES_WritePairs(ctx, g_init_array, init_size))
	{
		ctx->dev.state = GES_STATE_ERROR;
		return 0U;
	}

	delay_ms(50);

	if (!GES_WritePairs(ctx, g_gesture_array, ges_size))
	{
		ctx->dev.state = GES_STATE_ERROR;
		return 0U;
	}

	/* 驱动读手势寄存器基于BANK0，最后固定切回BANK0 */
	if (!GES_WriteReg(ctx, PAJ_REG_BANK_SEL, PAJ_BANK0))
	{
		ctx->dev.state = GES_STATE_ERROR;
		return 0U;
	}

	ctx->dev.state = GES_STATE_READY;
	ctx->dev.is_initialized = 1U;
	return 1U;
}

GES_handle_t GES_Create(GPIO_TypeDef *scl_port, uint16_t scl_pin, GPIO_TypeDef *sda_port, uint16_t sda_pin)
{
	uint8_t        i;
	GES_Context_t *ctx = 0;

	if (scl_port == 0 || sda_port == 0 || scl_pin == 0U || sda_pin == 0U)
	{
		/* 传入无效参数时，回退到默认引脚，便于快速联调 */
		scl_port = GES_I2C_SCL_PORT;
		scl_pin = GES_I2C_SCL_PIN;
		sda_port = GES_I2C_SDA_PORT;
		sda_pin = GES_I2C_SDA_PIN;
	}

	for (i = 0U; i < GES_MAX_DEVICES; i++)
	{
		if (g_ges_pool[i].in_use == 0U)
		{
			ctx = &g_ges_pool[i];
			ctx->in_use = 1U;
			break;
		}
	}
	if (ctx == 0)
	{
		return 0;
	}

	ctx->dev.scl_port = scl_port;
	ctx->dev.scl_pin = scl_pin;
	ctx->dev.sda_port = sda_port;
	ctx->dev.sda_pin = sda_pin;
	ctx->dev.state = GES_STATE_UNINIT;
	ctx->dev.is_initialized = 0U;
	ctx->i2c_bus = 0;

	if (!GES_InitDevice(ctx))
	{
		if (ctx->i2c_bus != 0)
		{
			(void)soft_i2c_master_del(ctx->i2c_bus);
			ctx->i2c_bus = 0;
		}
		ctx->in_use = 0U;
		return 0;
	}

	return &ctx->dev;
}

void GES_Destroy(GES_handle_t dev)
{
	GES_Context_t *ctx = GES_GetContext(dev);
	if (ctx == 0)
	{
		return;
	}

	if (ctx->i2c_bus != 0)
	{
		(void)soft_i2c_master_del(ctx->i2c_bus);
		ctx->i2c_bus = 0;
	}

	ctx->dev.state = GES_STATE_UNINIT;
	ctx->dev.is_initialized = 0U;
	ctx->in_use = 0U;
}

GES_State_t GES_get_state(GES_handle_t dev)
{
	GES_Context_t *ctx = GES_GetContext(dev);
	if (ctx == 0)
	{
		return GES_STATE_ERROR;
	}
	return ctx->dev.state;
}

GES_DataFlag_t GES_get_data(GES_handle_t dev)
{
	uint8_t        reg_data[2];
	uint16_t       ges_data;
	GES_Context_t *ctx = GES_GetContext(dev);

	if (ctx == 0 || GES_get_state(dev) != GES_STATE_READY)
	{
		return 0U;
	}

	/*
	 * 读取0x43/0x44会清除中断标志。
	 * 低字节为Up/Down/Left/Right/Forward/Backward/CW/CCW，
	 * 高字节bit0为Wave。
	 */
	if (!GES_ReadRegs(ctx, PAJ_REG_GES_INT_FLAG1, reg_data, 2U))
	{
		ctx->dev.state = GES_STATE_ERROR;
		return 0U;
	}

	ges_data = (uint16_t)reg_data[0];
	if ((reg_data[1] & 0x01U) != 0U)
	{
		ges_data |= GES_WAVE;
	}

	return (GES_DataFlag_t)ges_data;
}
