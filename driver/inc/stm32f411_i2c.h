/**
 * @file    stm32f411_i2c.h
 * @brief   Polling-mode I2C master driver for STM32F411xE.
 *
 * Supports 7-bit master transfers at standard mode (up to 100 kHz) and
 * fast mode (up to 400 kHz). The driver configures a fixed, valid pin pair
 * for each supported instance:
 * - I2C1: PB6 (SCL), PB7 (SDA), AF4
 * - I2C2: PB10 (SCL), PB11 (SDA), AF4
 * - I2C3: PA8 (SCL), PC9 (SDA), AF4
 *
 * The I2C bus requires external pull-up resistors. The internal GPIO pulls
 * are deliberately disabled because they are not suitable as bus pull-ups.
 */

#ifndef STM32F411_I2C_H
#define STM32F411_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f411_xe.h"

#define I2C_ERROR_INVALID_PARAM  (-1)
#define I2C_ERROR_TIMEOUT        (-2)
#define I2C_ERROR_UNSUPPORTED    (-3)
#define I2C_ERROR_BUSY           (-4)
#define I2C_ERROR_NACK           (-5)
#define I2C_ERROR_BUS            (-6)

/** I2C fast-mode duty-cycle selection. */
typedef enum {
  I2C_DUTY_CYCLE_2 = 0U,    /**< t_low/t_high = 2. */
  I2C_DUTY_CYCLE_16_9 = 1U /**< t_low/t_high = 16/9. */
} I2C_DutyCycle_t;

/** I2C master timing and own-address configuration. */
typedef struct {
  uint32_t clock_speed_hz; /**< Bus clock: 1..100000 or 100001..400000 Hz. */
  uint16_t own_address;    /**< Optional 7-bit address used when addressed as slave. */
  I2C_DutyCycle_t duty_cycle; /**< Fast-mode duty cycle; ignored in standard mode. */
} I2C_Config_t;

/**
 * @brief Initialize a supported I2C controller in 7-bit master mode.
 *
 * @param instance I2C1, I2C2, or I2C3.
 * @param config Timing and own-address settings.
 * @return 0 on success; I2C_ERROR_INVALID_PARAM or I2C_ERROR_UNSUPPORTED.
 */
int i2c_init(I2C_TypeDef *instance, const I2C_Config_t *config);

/**
 * @brief Send bytes to a 7-bit I2C slave and generate STOP.
 * @param instance Initialized I2C controller.
 * @param address 7-bit slave address (0x08..0x77).
 * @param data Bytes to send; may be NULL only when length is zero.
 * @param length Number of bytes to send; zero performs address-only probing.
 * @param timeout Polling-loop iteration limit; must be non-zero.
 * @return 0 or a documented I2C_ERROR_* code.
 */
int i2c_master_transmit(I2C_TypeDef *instance, uint16_t address,
                        const uint8_t *data, uint32_t length,
                        uint32_t timeout);

/**
 * @brief Receive bytes from a 7-bit I2C slave and generate STOP.
 * @param instance Initialized I2C controller.
 * @param address 7-bit slave address (0x08..0x77).
 * @param data Destination buffer.
 * @param length Number of bytes to receive; must be non-zero.
 * @param timeout Polling-loop iteration limit; must be non-zero.
 * @return 0 or a documented I2C_ERROR_* code.
 */
int i2c_master_receive(I2C_TypeDef *instance, uint16_t address,
                       uint8_t *data, uint32_t length, uint32_t timeout);

/**
 * @brief Read bytes from an 8-bit-register-addressed I2C slave.
 *
 * Sends @p register_address, then issues a repeated START before reading.
 * @param instance Initialized I2C controller.
 * @param address 7-bit slave address (0x08..0x77).
 * @param register_address 8-bit register/sub-address to read from.
 * @param data Destination buffer.
 * @param length Number of bytes to receive; must be non-zero.
 * @param timeout Polling-loop iteration limit; must be non-zero.
 * @return 0 or a documented I2C_ERROR_* code.
 */
int i2c_mem_read(I2C_TypeDef *instance, uint16_t address,
                 uint8_t register_address, uint8_t *data, uint32_t length,
                 uint32_t timeout);

/**
 * @brief Reset an I2C controller and release its APB1 reset line.
 * @param instance I2C1, I2C2, or I2C3.
 * @return 0 on success or I2C_ERROR_INVALID_PARAM.
 */
int i2c_deinit(I2C_TypeDef *instance);

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_I2C_H */
