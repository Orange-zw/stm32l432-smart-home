#include "mlx90614_dev.h"
#include "soft_i2c_master.h"
#include "stdlib.h"

#define MLX_I2C_ADDR_7BIT (0x5AU)
#define MLX_REG_TA (0x06U)
#define MLX_REG_TOBJ1 (0x07U)
#define MLX_I2C_FREQ (SOFT_I2C_100KHZ)
#define MLX_CTX_MAGIC (0x4D4C5831UL)
#define MLX_READ_RETRY_MAX (3U)
#define MLX_ERR_LATCH_THRESHOLD (5U)

typedef struct
{
	MLX_dev_t             dev;
	soft_i2c_master_bus_t i2c_bus;
	uint8_t               i2c_addr;
	uint8_t               fail_count;
	uint32_t              magic;
} MLX_Context_t;

static MLX_Context_t *MLX_GetContext(MLX_handle_t dev);
static uint8_t        MLX_CalcPec(const uint8_t *bytes, uint8_t len);
static uint8_t        MLX_ReadWord(MLX_Context_t *ctx, uint8_t reg, uint16_t *word);

static MLX_Context_t *MLX_GetContext(MLX_handle_t dev)
{
	MLX_Context_t *ctx;

	if (dev == 0)
	{
		return 0;
	}

	ctx = (MLX_Context_t *)dev;
	if (ctx->magic != MLX_CTX_MAGIC)
	{
		return 0;
	}
	return ctx;
}

static uint8_t MLX_CalcPec(const uint8_t *bytes, uint8_t len)
{
	uint8_t crc = 0U;
	uint8_t i;
	uint8_t j;

	if (bytes == 0 || len == 0U)
	{
		return 0U;
	}

	for (i = 0U; i < len; i++)
	{
		crc ^= bytes[i];
		for (j = 0U; j < 8U; j++)
		{
			if ((crc & 0x80U) != 0U)
			{
				crc = (uint8_t)((crc << 1) ^ 0x07U);
			}
			else
			{
				crc <<= 1;
			}
		}
	}

	return crc;
}

static uint8_t MLX_ReadWord(MLX_Context_t *ctx, uint8_t reg, uint16_t *word)
{
	uint8_t        rx[3];
	soft_i2c_err_t ret;
	uint8_t        pec_payload[5];
	uint8_t        expected_pec;

	if (ctx == 0 || word == 0 || ctx->i2c_bus == 0)
	{
		return 0U;
	}

	ret = soft_i2c_master_write_read(ctx->i2c_bus, ctx->i2c_addr, &reg, 1U, rx, 3U);
	if (ret != SOFT_I2C_OK)
	{
		return 0U;
	}

	pec_payload[0] = (uint8_t)(ctx->i2c_addr << 1);
	pec_payload[1] = reg;
	pec_payload[2] = (uint8_t)((ctx->i2c_addr << 1) | 0x01U);
	pec_payload[3] = rx[0];
	pec_payload[4] = rx[1];
	expected_pec = MLX_CalcPec(pec_payload, 5U);

	if (expected_pec != rx[2])
	{
		return 0U;
	}

	*word = (uint16_t)(((uint16_t)rx[1] << 8) | rx[0]);
	return 1U;
}

MLX_handle_t MLX_Init(GPIO_TypeDef *scl_port, uint16_t scl_pin, GPIO_TypeDef *sda_port, uint16_t sda_pin)
{
	MLX_Context_t           *ctx;
	soft_i2c_master_config_t i2c_cfg;
	uint16_t                 ta_raw = 0U;

	if (scl_port == 0 || sda_port == 0 || scl_pin == 0U || sda_pin == 0U)
	{
		return 0;
	}

	ctx = (MLX_Context_t *)malloc(sizeof(MLX_Context_t));
	if (ctx == 0)
	{
		return 0;
	}

	ctx->dev.scl_port = scl_port;
	ctx->dev.scl_pin = scl_pin;
	ctx->dev.sda_port = sda_port;
	ctx->dev.sda_pin = sda_pin;
	ctx->dev.state = MLX_STATE_UNINIT;
	ctx->dev.is_initialized = 0U;
	ctx->i2c_bus = 0;
	ctx->i2c_addr = MLX_I2C_ADDR_7BIT;
	ctx->fail_count = 0U;
	ctx->magic = MLX_CTX_MAGIC;

	i2c_cfg.scl_port = scl_port;
	i2c_cfg.scl_pin = scl_pin;
	i2c_cfg.sda_port = sda_port;
	i2c_cfg.sda_pin = sda_pin;
	i2c_cfg.freq = MLX_I2C_FREQ;

	if (soft_i2c_master_new(&i2c_cfg, &ctx->i2c_bus) != SOFT_I2C_OK)
	{
		ctx->dev.state = MLX_STATE_ERROR;
		ctx->magic = 0U;
		free(ctx);
		return 0;
	}

	/* 读取一次环境温度寄存器用于探测设备是否在线 */
	if (MLX_ReadWord(ctx, MLX_REG_TA, &ta_raw) == 0U)
	{
		(void)soft_i2c_master_del(ctx->i2c_bus);
		ctx->i2c_bus = 0;
		ctx->dev.state = MLX_STATE_ERROR;
		ctx->magic = 0U;
		free(ctx);
		return 0;
	}

	ctx->dev.state = MLX_STATE_READY;
	ctx->dev.is_initialized = 1U;
	return &ctx->dev;
}

void MLX_DeInit(MLX_handle_t dev)
{
	MLX_Context_t *ctx = MLX_GetContext(dev);
	if (ctx == 0)
	{
		return;
	}

	if (ctx->i2c_bus != 0)
	{
		(void)soft_i2c_master_del(ctx->i2c_bus);
		ctx->i2c_bus = 0;
	}

	ctx->dev.state = MLX_STATE_UNINIT;
	ctx->dev.is_initialized = 0U;
	ctx->magic = 0U;
	free(ctx);
}

uint8_t MLX_ReadTemp(MLX_handle_t dev, float *temp)
{
	uint16_t       raw;
	MLX_Context_t *ctx = MLX_GetContext(dev);
	uint8_t        retry;

	if (ctx == 0 || temp == 0 || ctx->dev.is_initialized == 0U)
	{
		return 0U;
	}

	for (retry = 0U; retry < MLX_READ_RETRY_MAX; retry++)
	{
		if (MLX_ReadWord(ctx, MLX_REG_TOBJ1, &raw) != 0U)
		{
			*temp = (float)raw * 0.02f - 273.15f;
			ctx->fail_count = 0U;
			ctx->dev.state = MLX_STATE_READY;
			return 1U;
		}
	}

	if (ctx->fail_count < 0xFFU)
	{
		ctx->fail_count++;
	}
	if (ctx->fail_count >= MLX_ERR_LATCH_THRESHOLD)
	{
		ctx->dev.state = MLX_STATE_ERROR;
	}
	return 0U;
}

MLX_State_t MLX_GetState(MLX_handle_t dev)
{
	MLX_Context_t *ctx = MLX_GetContext(dev);
	if (ctx == 0)
	{
		return MLX_STATE_ERROR;
	}
	return ctx->dev.state;
}
