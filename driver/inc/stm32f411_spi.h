/**
 * @file    stm32f411_spi.h
 * @brief   Polling-mode SPI master driver for STM32F411xE.
 *
 * Version 1 supports 8-bit, full-duplex master transfers on these fixed GPIO
 * mappings. The chip-select signal is intentionally software-controlled by
 * the application because it is specific to the attached device.
 *
 * - SPI1: PA5 (SCK), PA6 (MISO), PA7 (MOSI), AF5
 * - SPI2: PB13 (SCK), PB14 (MISO), PB15 (MOSI), AF5
 */

#ifndef STM32F411_SPI_H
#define STM32F411_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f411_xe.h"

#define SPI_ERROR_INVALID_PARAM  (-1)
#define SPI_ERROR_TIMEOUT        (-2)
#define SPI_ERROR_UNSUPPORTED    (-3)
#define SPI_ERROR_BUSY           (-4)
#define SPI_ERROR_OVERRUN        (-5)
#define SPI_ERROR_MODE_FAULT     (-6)

/** SPI clock polarity/phase mode (CPOL and CPHA). */
typedef enum {
  SPI_MODE_0 = 0U, /**< CPOL=0, CPHA=0. */
  SPI_MODE_1 = 1U, /**< CPOL=0, CPHA=1. */
  SPI_MODE_2 = 2U, /**< CPOL=1, CPHA=0. */
  SPI_MODE_3 = 3U  /**< CPOL=1, CPHA=1. */
} SPI_Mode_t;

/** SPI serial-clock prescaler relative to the controller PCLK. */
typedef enum {
  SPI_BAUDRATE_DIV2   = 0U, /**< PCLK / 2. */
  SPI_BAUDRATE_DIV4   = 1U, /**< PCLK / 4. */
  SPI_BAUDRATE_DIV8   = 2U, /**< PCLK / 8. */
  SPI_BAUDRATE_DIV16  = 3U, /**< PCLK / 16. */
  SPI_BAUDRATE_DIV32  = 4U, /**< PCLK / 32. */
  SPI_BAUDRATE_DIV64  = 5U, /**< PCLK / 64. */
  SPI_BAUDRATE_DIV128 = 6U, /**< PCLK / 128. */
  SPI_BAUDRATE_DIV256 = 7U  /**< PCLK / 256. */
} SPI_BaudRate_t;

/** SPI bit order for 8-bit transfers. */
typedef enum {
  SPI_BIT_ORDER_MSB_FIRST = 0U, /**< Transmit most-significant bit first. */
  SPI_BIT_ORDER_LSB_FIRST = 1U  /**< Transmit least-significant bit first. */
} SPI_BitOrder_t;

/** SPI master configuration. */
typedef struct {
  SPI_Mode_t mode;                /**< Required clock polarity and phase. */
  SPI_BaudRate_t baudrate;        /**< SCK divider relative to PCLK. */
  SPI_BitOrder_t bit_order;       /**< MSB-first or LSB-first frame order. */
} SPI_Config_t;

/**
 * @brief Initialize SPI1 or SPI2 for 8-bit, full-duplex master polling.
 *
 * The driver configures SCK, MISO, and MOSI using its documented fixed pin
 * mapping. It uses software NSS management; configure an application GPIO as
 * chip-select and drive it low around each transaction.
 *
 * @param instance SPI1 or SPI2.
 * @param config SPI timing and bit-order settings.
 * @return 0 on success; a documented SPI_ERROR_* code otherwise.
 */
int spi_init(SPI_TypeDef *instance, const SPI_Config_t *config);

/**
 * @brief Transfer an 8-bit full-duplex SPI frame sequence in polling mode.
 *
 * A NULL @p tx sends 0xFF dummy bytes. A NULL @p rx discards incoming bytes.
 * Both pointers may be NULL when @p length is zero.
 *
 * @param instance Initialized SPI1 or SPI2.
 * @param tx Source bytes, or NULL for dummy 0xFF bytes.
 * @param rx Destination bytes, or NULL to discard received bytes.
 * @param length Number of bytes to clock; may be zero.
 * @param timeout Per-status-flag polling-loop iteration limit; non-zero.
 * @return 0 on success; a documented SPI_ERROR_* code otherwise.
 */
int spi_transfer(SPI_TypeDef *instance, const uint8_t *tx, uint8_t *rx,
                 uint32_t length, uint32_t timeout);

/**
 * @brief Reset and disable SPI1 or SPI2.
 *
 * GPIO configuration is left unchanged so an application can reuse pins after
 * reinitializing the peripheral.
 *
 * @param instance SPI1 or SPI2.
 * @return 0 on success or SPI_ERROR_INVALID_PARAM.
 */
int spi_deinit(SPI_TypeDef *instance);

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_SPI_H */
