/**
 * @file    stm32f411_i2c.c
 * @brief   Polling-mode I2C master driver implementation for STM32F411xE.
 *
 * Reference: RM0383 Rev 3, Chapter 18.
 */

#include "stm32f411_i2c.h"
#include "stm32f411_gpio.h"
#include "stm32f411_rcc.h"
#include <stddef.h>

#define I2C_PCLK1_MIN_HZ 2000000U
#define I2C_PCLK1_MAX_HZ 50000000U
#define I2C_STANDARD_MAX_HZ 100000U
#define I2C_FAST_MAX_HZ 400000U
#define I2C_ERROR_FLAGS (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | \
                         I2C_SR1_OVR | I2C_SR1_TIMEOUT)

static int i2c_is_supported(const I2C_TypeDef *instance) {
  return (instance == I2C1) || (instance == I2C2) || (instance == I2C3);
}

static uint32_t i2c_reset_mask(const I2C_TypeDef *instance) {
  if (instance == I2C1) {
    return RCC_APB1RSTR_I2C1RST;
  }
  if (instance == I2C2) {
    return RCC_APB1RSTR_I2C2RST;
  }
  return RCC_APB1RSTR_I2C3RST;
}

static uint32_t i2c_enable_mask(const I2C_TypeDef *instance) {
  if (instance == I2C1) {
    return RCC_APB1ENR_I2C1EN;
  }
  if (instance == I2C2) {
    return RCC_APB1ENR_I2C2EN;
  }
  return RCC_APB1ENR_I2C3EN;
}

static int i2c_configure_pins(I2C_TypeDef *instance) {
  GPIO_TypeDef *scl_port;
  GPIO_TypeDef *sda_port;
  GPIO_Pin_t scl_pin;
  GPIO_Pin_t sda_pin;

  if (instance == I2C1) {
    scl_port = GPIOB; scl_pin = GPIO_PIN_6;
    sda_port = GPIOB; sda_pin = GPIO_PIN_7;
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIOBEN);
  } else if (instance == I2C2) {
    scl_port = GPIOB; scl_pin = GPIO_PIN_10;
    sda_port = GPIOB; sda_pin = GPIO_PIN_11;
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIOBEN);
  } else if (instance == I2C3) {
    scl_port = GPIOA; scl_pin = GPIO_PIN_8;
    sda_port = GPIOC; sda_pin = GPIO_PIN_9;
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN);
  } else {
    return I2C_ERROR_INVALID_PARAM;
  }

  GPIO_PinConfig_t scl_cfg = {
      .pin = scl_pin, .mode = GPIO_MODE_ALTFUNC,
      .otype = GPIO_OTYPE_OPENDRAIN, .ospeed = GPIO_OSPEED_FAST,
      .pupd = GPIO_PUPD_NONE, .alt_func = GPIO_AF4_I2C1_I2C3};
  GPIO_PinConfig_t sda_cfg = {
      .pin = sda_pin, .mode = GPIO_MODE_ALTFUNC,
      .otype = GPIO_OTYPE_OPENDRAIN, .ospeed = GPIO_OSPEED_FAST,
      .pupd = GPIO_PUPD_NONE, .alt_func = GPIO_AF4_I2C1_I2C3};

  if (gpio_init(scl_port, &scl_cfg) != 0) {
    return I2C_ERROR_INVALID_PARAM;
  }
  return (gpio_init(sda_port, &sda_cfg) == 0) ? 0 : I2C_ERROR_INVALID_PARAM;
}

static int i2c_check_error(I2C_TypeDef *instance) {
  uint32_t errors = instance->SR1 & I2C_ERROR_FLAGS;
  if (errors == 0U) {
    return 0;
  }
  instance->SR1 &= ~errors;
  return ((errors & I2C_SR1_AF) != 0U) ? I2C_ERROR_NACK : I2C_ERROR_BUS;
}

static int i2c_wait_sr1(I2C_TypeDef *instance, uint32_t flag,
                        uint32_t timeout) {
  while ((instance->SR1 & flag) == 0U) {
    int status = i2c_check_error(instance);
    if (status != 0) {
      return status;
    }
    if (timeout-- == 0U) {
      return I2C_ERROR_TIMEOUT;
    }
  }
  return i2c_check_error(instance);
}

static int i2c_wait_bus_idle(I2C_TypeDef *instance, uint32_t timeout) {
  while ((instance->SR2 & I2C_SR2_BUSY) != 0U) {
    int status = i2c_check_error(instance);
    if (status != 0) {
      return status;
    }
    if (timeout-- == 0U) {
      return I2C_ERROR_BUSY;
    }
  }
  return 0;
}

static void i2c_clear_addr(I2C_TypeDef *instance) {
  volatile uint32_t dummy;
  dummy = instance->SR1;
  dummy = instance->SR2;
  (void)dummy;
}

static int i2c_start(I2C_TypeDef *instance, uint32_t timeout) {
  instance->CR1 |= I2C_CR1_START;
  return i2c_wait_sr1(instance, I2C_SR1_SB, timeout);
}

static int i2c_send_address(I2C_TypeDef *instance, uint16_t address,
                            uint32_t read, uint32_t timeout) {
  instance->DR = ((uint32_t)address << 1U) | (read & 1U);
  return i2c_wait_sr1(instance, I2C_SR1_ADDR, timeout);
}

static void i2c_stop(I2C_TypeDef *instance) {
  instance->CR1 |= I2C_CR1_STOP;
}

static int i2c_validate_transfer(I2C_TypeDef *instance, uint16_t address,
                                 const void *data, uint32_t length,
                                 uint32_t timeout, uint32_t zero_allowed) {
  if (!i2c_is_supported(instance) || (address < 0x08U) ||
      (address > 0x77U) || (timeout == 0U) ||
      ((data == NULL) && ((length != 0U) || (zero_allowed == 0U)))) {
    return I2C_ERROR_INVALID_PARAM;
  }
  if ((instance->CR1 & I2C_CR1_PE) == 0U) {
    return I2C_ERROR_BUSY;
  }
  return 0;
}

static int i2c_receive_after_address(I2C_TypeDef *instance, uint8_t *data,
                                     uint32_t length, uint32_t timeout) {
  int status;

  if (length == 1U) {
    instance->CR1 &= ~(I2C_CR1_ACK | I2C_CR1_POS);
    i2c_clear_addr(instance);
    i2c_stop(instance);
    status = i2c_wait_sr1(instance, I2C_SR1_RXNE, timeout);
    if (status == 0) {
      data[0] = (uint8_t)instance->DR;
    }
  } else if (length == 2U) {
    instance->CR1 |= I2C_CR1_POS;
    instance->CR1 &= ~I2C_CR1_ACK;
    i2c_clear_addr(instance);
    status = i2c_wait_sr1(instance, I2C_SR1_BTF, timeout);
    if (status == 0) {
      i2c_stop(instance);
      data[0] = (uint8_t)instance->DR;
      data[1] = (uint8_t)instance->DR;
    }
  } else {
    instance->CR1 &= ~I2C_CR1_POS;
    instance->CR1 |= I2C_CR1_ACK;
    i2c_clear_addr(instance);

    uint32_t index = 0U;
    while ((length - index) > 3U) {
      status = i2c_wait_sr1(instance, I2C_SR1_RXNE, timeout);
      if (status != 0) {
        i2c_stop(instance);
        return status;
      }
      data[index++] = (uint8_t)instance->DR;
    }

    status = i2c_wait_sr1(instance, I2C_SR1_BTF, timeout);
    if (status == 0) {
      instance->CR1 &= ~I2C_CR1_ACK;
      data[index++] = (uint8_t)instance->DR;
      status = i2c_wait_sr1(instance, I2C_SR1_BTF, timeout);
      if (status == 0) {
        i2c_stop(instance);
        data[index++] = (uint8_t)instance->DR;
        data[index] = (uint8_t)instance->DR;
      }
    }
  }

  instance->CR1 &= ~(I2C_CR1_ACK | I2C_CR1_POS);
  if (status != 0) {
    i2c_stop(instance);
  }
  return status;
}

int i2c_init(I2C_TypeDef *instance, const I2C_Config_t *config) {
  if (!i2c_is_supported(instance) || (config == NULL) ||
      (config->clock_speed_hz == 0U) ||
      (config->clock_speed_hz > I2C_FAST_MAX_HZ) ||
      (config->own_address > 0x7FU) ||
      (config->duty_cycle > I2C_DUTY_CYCLE_16_9)) {
    return I2C_ERROR_INVALID_PARAM;
  }

  uint32_t pclk1_hz = rcc_get_pclk1_freq();
  if ((pclk1_hz < I2C_PCLK1_MIN_HZ) || (pclk1_hz > I2C_PCLK1_MAX_HZ) ||
      ((pclk1_hz % 1000000U) != 0U)) {
    return I2C_ERROR_UNSUPPORTED;
  }

  uint32_t pclk1_mhz = pclk1_hz / 1000000U;
  uint32_t ccr;
  uint32_t trise;
  uint32_t ccr_bits = 0U;

  if (config->clock_speed_hz <= I2C_STANDARD_MAX_HZ) {
    ccr = pclk1_hz / (2U * config->clock_speed_hz);
    if (ccr < 4U) {
      ccr = 4U;
    }
    trise = pclk1_mhz + 1U;
  } else {
    ccr_bits = I2C_CCR_FS;
    if (config->duty_cycle == I2C_DUTY_CYCLE_16_9) {
      ccr = pclk1_hz / (25U * config->clock_speed_hz);
      ccr_bits |= I2C_CCR_DUTY;
    } else {
      ccr = pclk1_hz / (3U * config->clock_speed_hz);
    }
    if (ccr == 0U) {
      return I2C_ERROR_UNSUPPORTED;
    }
    trise = ((pclk1_mhz * 3U) / 10U) + 1U;
  }

  if ((ccr > I2C_CCR_CCR_Msk) || (trise > I2C_TRISE_TRISE_Msk)) {
    return I2C_ERROR_UNSUPPORTED;
  }

  rcc_apb1_clk_enable(i2c_enable_mask(instance));
  rcc_apb1_periph_reset(i2c_reset_mask(instance));
  int status = i2c_configure_pins(instance);
  if (status != 0) {
    return status;
  }

  instance->CR1 &= ~I2C_CR1_PE;
  instance->CR2 = pclk1_mhz & I2C_CR2_FREQ_Msk;
  instance->OAR1 = I2C_OAR1_BIT14 | ((uint32_t)config->own_address << 1U);
  instance->OAR2 = 0U;
  instance->CCR = ccr_bits | (ccr & I2C_CCR_CCR_Msk);
  instance->TRISE = trise & I2C_TRISE_TRISE_Msk;
  instance->CR1 = I2C_CR1_PE | I2C_CR1_ACK;
  return 0;
}

int i2c_master_transmit(I2C_TypeDef *instance, uint16_t address,
                        const uint8_t *data, uint32_t length,
                        uint32_t timeout) {
  int status = i2c_validate_transfer(instance, address, data, length, timeout, 1U);
  if (status != 0) {
    return status;
  }
  status = i2c_wait_bus_idle(instance, timeout);
  if (status != 0) {
    return status;
  }

  instance->CR1 &= ~I2C_CR1_POS;
  instance->CR1 |= I2C_CR1_ACK;
  status = i2c_start(instance, timeout);
  if (status == 0) {
    status = i2c_send_address(instance, address, 0U, timeout);
  }
  if (status == 0) {
    i2c_clear_addr(instance);
    if (length != 0U) {
      for (uint32_t index = 0U; index < length; index++) {
        status = i2c_wait_sr1(instance, I2C_SR1_TXE, timeout);
        if (status != 0) {
          break;
        }
        instance->DR = data[index];
      }
      if (status == 0) {
        status = i2c_wait_sr1(instance, I2C_SR1_BTF, timeout);
      }
    }
  }
  i2c_stop(instance);
  return status;
}

int i2c_master_receive(I2C_TypeDef *instance, uint16_t address,
                       uint8_t *data, uint32_t length, uint32_t timeout) {
  int status = i2c_validate_transfer(instance, address, data, length, timeout, 0U);
  if ((status != 0) || (length == 0U)) {
    return (status != 0) ? status : I2C_ERROR_INVALID_PARAM;
  }
  status = i2c_wait_bus_idle(instance, timeout);
  if (status != 0) {
    return status;
  }
  status = i2c_start(instance, timeout);
  if (status == 0) {
    status = i2c_send_address(instance, address, 1U, timeout);
  }
  if (status != 0) {
    i2c_stop(instance);
    return status;
  }
  return i2c_receive_after_address(instance, data, length, timeout);
}

int i2c_mem_read(I2C_TypeDef *instance, uint16_t address,
                 uint8_t register_address, uint8_t *data, uint32_t length,
                 uint32_t timeout) {
  int status = i2c_validate_transfer(instance, address, data, length, timeout, 0U);
  if ((status != 0) || (length == 0U)) {
    return (status != 0) ? status : I2C_ERROR_INVALID_PARAM;
  }
  status = i2c_wait_bus_idle(instance, timeout);
  if (status != 0) {
    return status;
  }

  instance->CR1 &= ~I2C_CR1_POS;
  instance->CR1 |= I2C_CR1_ACK;
  status = i2c_start(instance, timeout);
  if (status == 0) {
    status = i2c_send_address(instance, address, 0U, timeout);
  }
  if (status == 0) {
    i2c_clear_addr(instance);
    status = i2c_wait_sr1(instance, I2C_SR1_TXE, timeout);
  }
  if (status == 0) {
    instance->DR = register_address;
    status = i2c_wait_sr1(instance, I2C_SR1_BTF, timeout);
  }
  if (status == 0) {
    status = i2c_start(instance, timeout);
  }
  if (status == 0) {
    status = i2c_send_address(instance, address, 1U, timeout);
  }
  if (status != 0) {
    i2c_stop(instance);
    return status;
  }
  return i2c_receive_after_address(instance, data, length, timeout);
}

int i2c_deinit(I2C_TypeDef *instance) {
  if (!i2c_is_supported(instance)) {
    return I2C_ERROR_INVALID_PARAM;
  }
  rcc_apb1_periph_reset(i2c_reset_mask(instance));
  rcc_apb1_clk_disable(i2c_enable_mask(instance));
  return 0;
}
