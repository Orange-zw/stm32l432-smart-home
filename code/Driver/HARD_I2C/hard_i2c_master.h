#ifndef __HARD_I2C_MASTER_H__
#define __HARD_I2C_MASTER_H__

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum
	{
		HARD_I2C_100KHZ = 0,
		HARD_I2C_400KHZ,
		HARD_I2C_FREQ_END
	} hard_i2c_master_freq_t;

	typedef enum
	{
		HARD_I2C_OK = 0,
		HARD_I2C_ERR_INVALID_ARG = -1,
		HARD_I2C_ERR_NO_MEM = -2,
		HARD_I2C_ERR_TIMEOUT = -3,
		HARD_I2C_ERR_NOT_FOUND = -4,
		HARD_I2C_ERR_FAIL = -5
	} hard_i2c_err_t;

	/*
	 * 硬件I2C配置：
	 * - instance: I2C1 / I2C2
	 * - freq: 目标总线速率档位
	 * - remap_enable: I2C1是否开启重映射（0: PB6/PB7, 1: PB8/PB9），I2C2忽略
	 */
	typedef struct
	{
		I2C_TypeDef           *instance;
		hard_i2c_master_freq_t freq;
		uint8_t                remap_enable;
	} hard_i2c_master_config_t;

	typedef struct hard_i2c_master_bus_impl_t *hard_i2c_master_bus_t;

	hard_i2c_err_t hard_i2c_master_new(const hard_i2c_master_config_t *config, hard_i2c_master_bus_t *bus);
	hard_i2c_err_t hard_i2c_master_del(hard_i2c_master_bus_t bus);

	hard_i2c_err_t hard_i2c_master_write(
	    hard_i2c_master_bus_t bus,
	    uint8_t               device_address,
	    const uint8_t        *write_buffer,
	    uint16_t              write_size);

	hard_i2c_err_t hard_i2c_master_read(
	    hard_i2c_master_bus_t bus,
	    uint8_t               device_address,
	    uint8_t              *read_buffer,
	    uint16_t              read_size);

	hard_i2c_err_t hard_i2c_master_write_read(
	    hard_i2c_master_bus_t bus,
	    uint8_t               device_address,
	    const uint8_t        *write_buffer,
	    uint16_t              write_size,
	    uint8_t              *read_buffer,
	    uint16_t              read_size);

#ifdef __cplusplus
}
#endif

#endif /* __HARD_I2C_MASTER_H__ */
