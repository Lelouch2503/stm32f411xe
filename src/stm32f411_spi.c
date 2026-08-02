/**
 * @file    stm32f411_spi.c
 * @brief   Polling-mode SPI master driver implementation for STM32F411xE.
 *
 * Provides functions for SPI peripheral clock configuration, GPIO pin assignment,
 * master control register initialization, full-duplex byte transfers, and hardware reset.
 *
 * Reference: RM0383 Rev 3, Chapter 20 (Serial Peripheral Interface - SPI).
 */

#include "stm32f411_spi.h"
#include "stm32f411_gpio.h"
#include "stm32f411_rcc.h"
#include <stddef.h>

/* ========================================================================== */
/* Private Helper Functions                                                   */
/* ========================================================================== */

/**
 * @brief  Check if the requested SPI peripheral instance is supported by this driver.
 *
 * @param  instance  SPI peripheral base pointer (SPI1 or SPI2).
 * @return Non-zero (1) if instance is SPI1 or SPI2; 0 otherwise.
 */
static int spi_is_supported(const SPI_TypeDef *instance) {
  return (instance == SPI1) || (instance == SPI2);
}

/**
 * @brief  Configure GPIO pins for the designated SPI peripheral in alternate-function mode.
 *
 * Pin Mappings:
 *  - SPI1: PA5 (SCK), PA6 (MISO), PA7 (MOSI) using AF5 (SPI1/SPI5)
 *  - SPI2: PB13 (SCK), PB14 (MISO), PB15 (MOSI) using AF5 (SPI1/SPI5)
 *
 * Pin Configuration:
 *  - Mode: Alternate Function (GPIO_MODE_ALTFUNC)
 *  - Output Type: Push-Pull (GPIO_OTYPE_PUSHPULL)
 *  - Speed: High Speed (GPIO_OSPEED_HIGH)
 *  - Pull: None (GPIO_PUPD_NONE)
 *
 * @param  instance  SPI peripheral base pointer.
 * @return 0 on success, or SPI_ERROR_INVALID_PARAM on invalid instance or pin init failure.
 */
static int spi_configure_pins(SPI_TypeDef *instance) {
  GPIO_TypeDef *port;
  GPIO_Pin_t sck_pin;
  GPIO_Pin_t miso_pin;
  GPIO_Pin_t mosi_pin;

  /* 1. Identify port and pins, and enable the corresponding GPIO port clock */
  if (instance == SPI1) {
    port = GPIOA;
    sck_pin = GPIO_PIN_5;
    miso_pin = GPIO_PIN_6;
    mosi_pin = GPIO_PIN_7;
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIOAEN);
  } else if (instance == SPI2) {
    port = GPIOB;
    sck_pin = GPIO_PIN_13;
    miso_pin = GPIO_PIN_14;
    mosi_pin = GPIO_PIN_15;
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIOBEN);
  } else {
    return SPI_ERROR_INVALID_PARAM;
  }

  /* 2. Set up base pin configuration for SPI signal lines */
  const GPIO_PinConfig_t pin_cfg = {
      .mode = GPIO_MODE_ALTFUNC,
      .otype = GPIO_OTYPE_PUSHPULL,
      .ospeed = GPIO_OSPEED_HIGH,
      .pupd = GPIO_PUPD_NONE,
      .alt_func = GPIO_AF5_SPI1_SPI5,
  };
  GPIO_PinConfig_t config = pin_cfg;

  /* 3. Initialize SCK pin */
  config.pin = sck_pin;
  if (gpio_init(port, &config) != 0) {
    return SPI_ERROR_INVALID_PARAM;
  }

  /* 4. Initialize MISO pin */
  config.pin = miso_pin;
  if (gpio_init(port, &config) != 0) {
    return SPI_ERROR_INVALID_PARAM;
  }

  /* 5. Initialize MOSI pin */
  config.pin = mosi_pin;
  return (gpio_init(port, &config) == 0) ? 0 : SPI_ERROR_INVALID_PARAM;
}

/**
 * @brief  Enable the RCC bus clock for the specified SPI instance.
 *
 * SPI1 is located on APB2; SPI2 is located on APB1.
 *
 * @param  instance  SPI peripheral base pointer.
 */
static void spi_enable_clock(SPI_TypeDef *instance) {
  if (instance == SPI1) {
    rcc_apb2_clk_enable(RCC_APB2ENR_SPI1EN);
  } else {
    rcc_apb1_clk_enable(RCC_APB1ENR_SPI2EN);
  }
}

/**
 * @brief  Pulse peripheral reset for the specified SPI instance.
 *
 * Triggers a software reset pulse via RCC reset registers to restore hardware state.
 *
 * @param  instance  SPI peripheral base pointer.
 */
static void spi_reset(SPI_TypeDef *instance) {
  if (instance == SPI1) {
    rcc_apb2_periph_reset(RCC_APB2RSTR_SPI1RST);
  } else {
    rcc_apb1_periph_reset(RCC_APB1RSTR_SPI2RST);
  }
}

/**
 * @brief  Disable the RCC bus clock for the specified SPI instance.
 *
 * @param  instance  SPI peripheral base pointer.
 */
static void spi_disable_clock(SPI_TypeDef *instance) {
  if (instance == SPI1) {
    rcc_apb2_clk_disable(RCC_APB2ENR_SPI1EN);
  } else {
    rcc_apb1_clk_disable(RCC_APB1ENR_SPI2EN);
  }
}

/**
 * @brief  Clear an overrun (OVR) error condition on the SPI hardware.
 *
 * According to STM32F4 reference manual (RM0383), an OVR flag is cleared by a
 * read access to SPI_DR followed by a read access to SPI_SR.
 *
 * @param  instance  SPI peripheral base pointer.
 */
static void spi_clear_overrun(SPI_TypeDef *instance) {
  volatile uint32_t dummy;

  dummy = instance->DR;
  dummy = instance->SR;
  (void)dummy;
}

/**
 * @brief  Inspect SPI status register for pending hardware error flags.
 *
 * Checks for Overrun (OVR) and Mode Fault (MODF) flags and performs recovery sequences.
 *
 * @param  instance  SPI peripheral base pointer.
 * @return 0 if no error, SPI_ERROR_OVERRUN, or SPI_ERROR_MODE_FAULT.
 */
static int spi_check_error(SPI_TypeDef *instance) {
  uint32_t status = instance->SR;

  if ((status & SPI_SR_OVR) != 0U) {
    spi_clear_overrun(instance);
    return SPI_ERROR_OVERRUN;
  }
  if ((status & SPI_SR_MODF) != 0U) {
    /* MODF clears after the SR read above followed by a CR1 write.
     * Force Internal Slave Select (SSI) bit high to recover master mode. */
    instance->CR1 |= SPI_CR1_SSI;
    return SPI_ERROR_MODE_FAULT;
  }
  return 0;
}

/**
 * @brief  Poll until a specific status flag in SPI_SR is set or timeout occurs.
 *
 * @param  instance  SPI peripheral base pointer.
 * @param  flag      Status register bit flag to monitor (e.g. SPI_SR_TXE, SPI_SR_RXNE).
 * @param  timeout   Maximum polling loop iterations.
 * @return 0 on success, SPI_ERROR_TIMEOUT, or hardware error code.
 */
static int spi_wait_flag(SPI_TypeDef *instance, uint32_t flag,
                         uint32_t timeout) {
  while ((instance->SR & flag) == 0U) {
    int status = spi_check_error(instance);
    if (status != 0) {
      return status;
    }
    if (timeout-- == 0U) {
      return SPI_ERROR_TIMEOUT;
    }
  }
  return spi_check_error(instance);
}

/**
 * @brief  Poll until the SPI peripheral is no longer busy (BSY flag clear).
 *
 * @param  instance  SPI peripheral base pointer.
 * @param  timeout   Maximum polling loop iterations.
 * @return 0 when not busy, SPI_ERROR_BUSY on timeout, or hardware error code.
 */
static int spi_wait_not_busy(SPI_TypeDef *instance, uint32_t timeout) {
  while ((instance->SR & SPI_SR_BSY) != 0U) {
    int status = spi_check_error(instance);
    if (status != 0) {
      return status;
    }
    if (timeout-- == 0U) {
      return SPI_ERROR_BUSY;
    }
  }
  return spi_check_error(instance);
}

/* ========================================================================== */
/* Public API Implementations                                                 */
/* ========================================================================== */

/**
 * @brief  Initialize SPI instance as Master in 8-bit full-duplex mode.
 *
 * Configures RCC clocks, resets peripheral, sets up alternate function GPIOs,
 * and sets up control register 1 (CR1) for master mode, software slave management,
 * clock polarity/phase, baud rate, and bit frame order.
 *
 * @param  instance  SPI peripheral base pointer (SPI1 or SPI2).
 * @param  config    Pointer to SPI configuration structure.
 * @return 0 on success, or SPI_ERROR_INVALID_PARAM on parameter check or GPIO failure.
 */
int spi_init(SPI_TypeDef *instance, const SPI_Config_t *config) {
  /* 1. Validate input parameters */
  if (!spi_is_supported(instance) || (config == NULL) ||
      (config->mode > SPI_MODE_3) ||
      (config->baudrate > SPI_BAUDRATE_DIV256) ||
      (config->bit_order > SPI_BIT_ORDER_LSB_FIRST)) {
    return SPI_ERROR_INVALID_PARAM;
  }

  /* 2. Enable peripheral RCC clock and issue software reset */
  spi_enable_clock(instance);
  spi_reset(instance);

  /* 3. Configure GPIO pins (SCK, MISO, MOSI) for the selected instance */
  int status = spi_configure_pins(instance);
  if (status != 0) {
    return status;
  }

  /* 4. Prepare Control Register 1 (CR1) settings:
   *    - MSTR: Master configuration
   *    - SSM: Software slave management enabled
   *    - SSI: Internal slave select set high (prevents MODF in master mode)
   *    - BR[2:0]: Baud rate prescaler configuration
   */
  uint32_t cr1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                 ((uint32_t)config->baudrate << SPI_CR1_BR_Pos);
  if ((config->mode & 1U) != 0U) {
    cr1 |= SPI_CR1_CPHA;
  }
  if ((config->mode & 2U) != 0U) {
    cr1 |= SPI_CR1_CPOL;
  }
  if (config->bit_order == SPI_BIT_ORDER_LSB_FIRST) {
    cr1 |= SPI_CR1_LSBFIRST;
  }

  /* 5. Clear registers to known default states while SPI is disabled */
  instance->CR1 = 0U;
  instance->CR2 = 0U;
  instance->I2SCFGR = 0U;
  instance->CRCPR = 7U;

  /* 6. Write CR1 configuration and enable SPI (SPE bit) */
  instance->CR1 = cr1 | SPI_CR1_SPE;
  return 0;
}

/**
 * @brief  Perform a synchronous polling full-duplex transfer over SPI.
 *
 * For each byte of the payload:
 *  - Waits for TXE (Transmit buffer empty).
 *  - Writes byte to SPI_DR (or dummy 0xFF if tx is NULL).
 *  - Waits for RXNE (Receive buffer not empty).
 *  - Reads byte from SPI_DR (stores in rx if rx is not NULL).
 * Finally waits for the BSY flag to clear.
 *
 * @param  instance  SPI peripheral base pointer (SPI1 or SPI2).
 * @param  tx        Buffer of bytes to transmit (or NULL to send 0xFF dummy bytes).
 * @param  rx        Buffer to store received bytes (or NULL to discard received data).
 * @param  length    Number of bytes to transfer.
 * @param  timeout   Polling timeout limit per wait iteration.
 * @return 0 on success, SPI_ERROR_INVALID_PARAM, SPI_ERROR_BUSY, SPI_ERROR_TIMEOUT,
 *         SPI_ERROR_OVERRUN, or SPI_ERROR_MODE_FAULT.
 */
int spi_transfer(SPI_TypeDef *instance, const uint8_t *tx, uint8_t *rx,
                 uint32_t length, uint32_t timeout) {
  /* 1. Validate parameters and ensure peripheral is enabled */
  if (!spi_is_supported(instance) || (timeout == 0U)) {
    return SPI_ERROR_INVALID_PARAM;
  }
  if ((instance->CR1 & SPI_CR1_SPE) == 0U) {
    return SPI_ERROR_BUSY;
  }

  /* 2. Clear any lingering error states before starting transfer */
  int status = spi_check_error(instance);
  if (status != 0) {
    return status;
  }

  /* 3. Byte-by-byte full-duplex SPI transfer loop */
  for (uint32_t index = 0U; index < length; index++) {
    /* Step 3a: Wait until transmit buffer is empty (TXE flag) */
    status = spi_wait_flag(instance, SPI_SR_TXE, timeout);
    if (status != 0) {
      return status;
    }

    /* Step 3b: Load byte into DR register to initiate transfer clocking */
    instance->DR = (tx != NULL) ? tx[index] : 0xFFU;

    /* Step 3c: Wait until receive buffer contains data (RXNE flag) */
    status = spi_wait_flag(instance, SPI_SR_RXNE, timeout);
    if (status != 0) {
      return status;
    }

    /* Step 3d: Read received byte from DR register */
    uint8_t received = (uint8_t)instance->DR;
    if (rx != NULL) {
      rx[index] = received;
    }
  }

  /* 4. Wait for SPI engine to complete last frame (BSY flag clear) */
  return spi_wait_not_busy(instance, timeout);
}

/**
 * @brief  De-initialize the SPI peripheral by resetting its registers and disabling its clock.
 *
 * @param  instance  SPI peripheral base pointer (SPI1 or SPI2).
 * @return 0 on success, or SPI_ERROR_INVALID_PARAM if instance is unsupported.
 */
int spi_deinit(SPI_TypeDef *instance) {
  /* 1. Validate instance */
  if (!spi_is_supported(instance)) {
    return SPI_ERROR_INVALID_PARAM;
  }

  /* 2. Reset peripheral hardware registers and disable peripheral clock */
  spi_reset(instance);
  spi_disable_clock(instance);
  return 0;
}

