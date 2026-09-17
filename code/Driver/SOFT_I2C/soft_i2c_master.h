#ifndef __SOFT_I2C_MASTER_H__
#define __SOFT_I2C_MASTER_H__

#include "stm32f10x.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief 软件I2C主机支持的目标频率档位（近似值）
	 */
	typedef enum
	{
		SOFT_I2C_100KHZ = 0,
		SOFT_I2C_200KHZ,
		SOFT_I2C_300KHZ,
		SOFT_I2C_FREQ_END
	} soft_i2c_master_freq_t;

	/**
	 * @brief 软件I2C返回值
	 */
	typedef enum
	{
		SOFT_I2C_OK = 0,
		SOFT_I2C_ERR_INVALID_ARG = -1,
		SOFT_I2C_ERR_NO_MEM = -2,
		SOFT_I2C_ERR_NOT_FOUND = -3, /* 地址阶段未应答 */
		SOFT_I2C_ERR_FAIL = -4       /* 数据阶段失败（NACK） */
	} soft_i2c_err_t;

	/**
	 * @brief 软件I2C总线配置
	 */
	typedef struct
	{
		GPIO_TypeDef          *scl_port;
		uint16_t               scl_pin;
		GPIO_TypeDef          *sda_port;
		uint16_t               sda_pin;
		soft_i2c_master_freq_t freq;
	} soft_i2c_master_config_t;

	/**
	 * @brief 软件I2C总线句柄
	 */
	typedef struct i2c_master_bus_impl_t *soft_i2c_master_bus_t;

	/**
	 * @brief 创建并初始化软件I2C总线
	 */
	soft_i2c_err_t soft_i2c_master_new(const soft_i2c_master_config_t *config, soft_i2c_master_bus_t *bus);

	/**
	 * @brief 删除软件I2C总线
	 */
	soft_i2c_err_t soft_i2c_master_del(soft_i2c_master_bus_t bus);

	/**
	 * @brief 仅写传输（START + DEV(W) + DATA... + STOP）
	 */
	soft_i2c_err_t soft_i2c_master_write(
	    soft_i2c_master_bus_t bus,
	    uint8_t               device_address,
	    const uint8_t        *write_buffer,
	    uint16_t              write_size);

	/**
	 * @brief 仅读传输（START + DEV(R) + DATA... + STOP）
	 */
	soft_i2c_err_t soft_i2c_master_read(
	    soft_i2c_master_bus_t bus,
	    uint8_t               device_address,
	    uint8_t              *read_buffer,
	    uint16_t              read_size);

	/**
	 * @brief 写后读传输（START + DEV(W) + WDATA... + RESTART + DEV(R) + RDATA... + STOP）
	 */
	soft_i2c_err_t soft_i2c_master_write_read(
	    soft_i2c_master_bus_t bus,
	    uint8_t               device_address,
	    const uint8_t        *write_buffer,
	    uint16_t              write_size,
	    uint8_t              *read_buffer,
	    uint16_t              read_size);

#ifdef __cplusplus
}
#endif

#endif /* __SOFT_I2C_MASTER_H__ */
