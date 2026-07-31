/**
 * @file    stm32f411_usart.c
 * @brief   USART driver implementation for STM32F411xE
 *
 * Implements USART initialization (with automatic GPIO configuration),
 * baud rate calculation, polling transmit/receive, and de-initialization.
 *
 * Supported instances: USART1, USART2, USART6
 *
 * Reference: RM0383 Rev 3 — STM32F411xC/E Reference Manual, Chapter 19
 */

#include "stm32f411_usart.h"
#include "stm32f411_gpio.h"
#include "stm32f411_rcc.h"
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════
 * Private Constants
 * ═════════════════════════════════════════════════════════════════════ */

/** Default timeout for internal polling loops (iterations) */
#define USART_DEFAULT_TIMEOUT 0x10000U

/* ══════════════════════════════════════════════════════════════════════
 * Private Helpers
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Get the peripheral clock frequency for a given USART instance.
 *
 * USART2 is on APB1, USART1 and USART6 are on APB2.
 *
 * @param  instance  USART peripheral pointer.
 * @return Peripheral clock frequency in Hz, or 0 if instance is invalid.
 */
static uint32_t usart_get_pclk(const USART_TypeDef *instance) {
  if (instance == USART2) {
    return rcc_get_pclk1_freq();
  } else if ((instance == USART1) || (instance == USART6)) {
    return rcc_get_pclk2_freq();
  }
  return 0U;
}

/**
 * @brief  Configure GPIO pins for a USART instance.
 *
 * Enables the GPIO port clock and configures TX/RX pins as
 * alternate function push-pull with pull-up. Uses default pin mapping:
 *   USART1 — PA9  (TX), PA10 (RX), AF7
 *   USART2 — PA2  (TX), PA3  (RX), AF7
 *   USART6 — PA11 (TX), PA12 (RX), AF8
 *
 * @param  instance  USART peripheral pointer.
 * @retval  0   Success.
 * @retval -1   Unsupported instance.
 */
static int usart_configure_gpio(const USART_TypeDef *instance) {
  GPIO_TypeDef *gpio_port;
  GPIO_Pin_t tx_pin;
  GPIO_Pin_t rx_pin;
  GPIO_AltFunc_t af;

  if (instance == USART1) {
    gpio_port = GPIOA;
    tx_pin = GPIO_PIN_9;
    rx_pin = GPIO_PIN_10;
    af = GPIO_AF7_USART1_2;
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIOAEN);
  } else if (instance == USART2) {
    gpio_port = GPIOA;
    tx_pin = GPIO_PIN_2;
    rx_pin = GPIO_PIN_3;
    af = GPIO_AF7_USART1_2;
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIOAEN);
  } else if (instance == USART6) {
    gpio_port = GPIOA;
    tx_pin = GPIO_PIN_11;
    rx_pin = GPIO_PIN_12;
    af = GPIO_AF8_USART6;
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIOAEN);
  } else {
    return -1;
  }

  /* Configure TX pin: Alternate Function, Push-Pull, Fast speed, Pull-Up */
  GPIO_PinConfig_t tx_cfg = {
    .pin = tx_pin,
    .mode = GPIO_MODE_ALTFUNC,
    .otype = GPIO_OTYPE_PUSHPULL,
    .ospeed = GPIO_OSPEED_FAST,
    .pupd = GPIO_PUPD_PULLUP,
    .alt_func = af
  };
  int rc = gpio_init(gpio_port, &tx_cfg);
  if (rc != 0) {
    return rc;
  }

  /* Configure RX pin: Alternate Function, Push-Pull, Fast speed, Pull-Up */
  GPIO_PinConfig_t rx_cfg = {
    .pin = rx_pin,
    .mode = GPIO_MODE_ALTFUNC,
    .otype = GPIO_OTYPE_PUSHPULL,
    .ospeed = GPIO_OSPEED_FAST,
    .pupd = GPIO_PUPD_PULLUP,
    .alt_func = af
  };
  return gpio_init(gpio_port, &rx_cfg);
}

/**
 * @brief  Enable the peripheral clock for a USART instance.
 *
 * @param  instance  USART peripheral pointer.
 */
static void usart_enable_clock(const USART_TypeDef *instance) {
  if (instance == USART1) {
    rcc_apb2_clk_enable(RCC_APB2ENR_USART1EN);
  } else if (instance == USART2) {
    rcc_apb1_clk_enable(RCC_APB1ENR_USART2EN);
  } else if (instance == USART6) {
    rcc_apb2_clk_enable(RCC_APB2ENR_USART6EN);
  }
}

/**
 * @brief  Calculate and write the baud rate register (BRR).
 *
 * Formula (RM0383 §19.3.4):
 *   OVER8=0: USARTDIV = fCK / (16 × baud)
 *   OVER8=1: USARTDIV = fCK / (8  × baud)
 *
 * The mantissa and fraction are packed into BRR:
 *   BRR[15:4] = mantissa, BRR[3:0] = fraction
 *   For OVER8=1, BRR[2:0] = fraction[2:0], BRR[3] must be kept clear.
 *
 * @param  instance      USART peripheral pointer.
 * @param  pclk          Peripheral clock frequency in Hz.
 * @param  baudrate      Desired baud rate.
 * @param  oversampling  Oversampling mode (16x or 8x).
 * @retval  0   Success.
 * @retval -3   BRR is zero (baud rate too high for the given clock).
 */
static int usart_set_baudrate(USART_TypeDef *instance, uint32_t pclk,
                              uint32_t baudrate,
                              USART_OverSampling_t oversampling) {
  uint32_t usartdiv;

  if (oversampling == USART_OVERSAMPLING_8) {
    /* USARTDIV × 8 = fCK / baud
     * Multiply by 2 for fixed-point: (2 × fCK) / baud
     * Then mantissa = result >> 4, fraction = (result & 0xF) >> 1 */
    usartdiv = ((2U * pclk) + (baudrate / 2U)) / baudrate;
    uint32_t mantissa = usartdiv >> 4U;
    uint32_t fraction = (usartdiv & 0x0FU) >> 1U; /* 3-bit fraction */
    uint32_t brr = (mantissa << USART_BRR_DIV_Mantissa_Pos) |
                   (fraction << USART_BRR_DIV_Fraction_Pos);
    if (brr == 0U) {
      return -3;
    }
    instance->BRR.reg = brr;
  } else {
    /* USARTDIV × 16 = fCK / baud
     * Standard fixed-point: (fCK + baud/2) / baud for rounding
     * mantissa = result >> 4, fraction = result & 0xF */
    usartdiv = (pclk + (baudrate / 2U)) / baudrate;
    if (usartdiv == 0U) {
      return -3;
    }
    instance->BRR.reg = usartdiv;
  }

  return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * Public API Implementation
 * ═════════════════════════════════════════════════════════════════════ */

int usart_init(USART_TypeDef *instance, const USART_Config_t *config) {
  /* 1. Validate parameters */
  if ((instance == NULL) || (config == NULL)) {
    return -1;
  }
  if ((instance != USART1) && (instance != USART2) && (instance != USART6)) {
    return -1;
  }
  if (config->baudrate == 0U) {
    return -1;
  }
  if (config->mode == 0U) {
    return -1;
  }

  /* 2. Enable peripheral clock */
  usart_enable_clock(instance);

  /* 3. Configure GPIO TX/RX pins */
  int rc = usart_configure_gpio(instance);
  if (rc != 0) {
    return rc;
  }

  /* 4. Disable USART before configuration (UE = 0) */
  instance->CR1.bit.UE = 0U;

  /* 5. Configure CR1: word length, parity, oversampling, mode */
  {
    uint32_t cr1 = 0U;

    /* Word length */
    if (config->word_length == USART_WORDLEN_9BIT) {
      cr1 |= USART_CR1_M;
    }

    /* Parity */
    if (config->parity != USART_PARITY_NONE) {
      cr1 |= USART_CR1_PCE;
      if (config->parity == USART_PARITY_ODD) {
        cr1 |= USART_CR1_PS;
      }
    }

    /* Oversampling */
    if (config->oversampling == USART_OVERSAMPLING_8) {
      cr1 |= USART_CR1_OVER8;
    }

    /* TX/RX enable */
    if (config->mode & USART_MODE_TX) {
      cr1 |= USART_CR1_TE;
    }
    if (config->mode & USART_MODE_RX) {
      cr1 |= USART_CR1_RE;
    }

    instance->CR1.reg = cr1;
  }

  /* 6. Configure CR2: stop bits */
  {
    uint32_t cr2 = instance->CR2.reg;
    cr2 &= ~USART_CR2_STOP_Msk;
    cr2 |= ((uint32_t)config->stop_bits << USART_CR2_STOP_Pos);
    instance->CR2.reg = cr2;
  }

  /* 7. Configure CR3: hardware flow control */
  {
    uint32_t cr3 = instance->CR3.reg;
    cr3 &= ~(USART_CR3_RTSE | USART_CR3_CTSE);
    if (config->hw_flow_ctl & USART_HWFLOW_RTS) {
      cr3 |= USART_CR3_RTSE;
    }
    if (config->hw_flow_ctl & USART_HWFLOW_CTS) {
      cr3 |= USART_CR3_CTSE;
    }
    instance->CR3.reg = cr3;
  }

  /* 8. Calculate and set baud rate */
  uint32_t pclk = usart_get_pclk(instance);
  if (pclk == 0U) {
    return -1;
  }
  rc = usart_set_baudrate(instance, pclk, config->baudrate,
                          config->oversampling);
  if (rc != 0) {
    return rc;
  }

  /* 9. Clear stale status flags by reading SR then DR */
  (void)instance->SR.reg;
  (void)instance->DR.reg;

  /* 10. Enable USART (UE = 1) */
  instance->CR1.bit.UE = 1U;

  return 0;
}

int usart_deinit(USART_TypeDef *instance) {
  if (instance == NULL) {
    return -1;
  }

  /* Disable USART */
  instance->CR1.bit.UE = 0U;

  /* Reset peripheral via RCC */
  if (instance == USART1) {
    rcc_apb2_periph_reset(RCC_APB2RSTR_USART1RST);
  } else if (instance == USART2) {
    rcc_apb1_periph_reset(RCC_APB1RSTR_USART2RST);
  } else if (instance == USART6) {
    rcc_apb2_periph_reset(RCC_APB2RSTR_USART6RST);
  } else {
    return -1;
  }

  return 0;
}

int usart_transmit(USART_TypeDef *instance, const uint8_t *data,
                   uint32_t len, uint32_t timeout) {
  if ((instance == NULL) || (data == NULL)) {
    return -1;
  }

  for (uint32_t i = 0U; i < len; i++) {
    /* Wait for TXE (Transmit Data Register Empty) */
    uint32_t count = timeout;
    while (!instance->SR.bit.TXE) {
      if (timeout != 0U) {
        if (--count == 0U) {
          return -2;
        }
      }
    }

    /* Write data byte to DR */
    instance->DR.reg = data[i];
  }

  /* Wait for TC (Transmission Complete) — last byte fully shifted out */
  {
    uint32_t count = timeout;
    while (!instance->SR.bit.TC) {
      if (timeout != 0U) {
        if (--count == 0U) {
          return -2;
        }
      }
    }
  }

  return 0;
}

int usart_receive(USART_TypeDef *instance, uint8_t *data,
                  uint32_t len, uint32_t timeout) {
  if ((instance == NULL) || (data == NULL)) {
    return -1;
  }

  for (uint32_t i = 0U; i < len; i++) {
    /* Wait for RXNE (Read Data Register Not Empty) */
    uint32_t count = timeout;
    while (!instance->SR.bit.RXNE) {
      /* Check for hardware errors before timeout */
      uint32_t sr = instance->SR.reg;
      if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NF | USART_SR_PE)) {
        /* Clear errors by reading SR then DR (RM0383 §19.6.1) */
        (void)instance->DR.reg;
        return -5;
      }

      if (timeout != 0U) {
        if (--count == 0U) {
          return -2;
        }
      }
    }

    /* Read data byte from DR (also clears RXNE) */
    data[i] = (uint8_t)(instance->DR.reg & 0xFFU);
  }

  return 0;
}

int usart_write_byte(USART_TypeDef *instance, uint8_t byte, uint32_t timeout) {
  return usart_transmit(instance, &byte, 1U, timeout);
}

int usart_read_byte(USART_TypeDef *instance, uint8_t *byte, uint32_t timeout) {
  if (byte == NULL) {
    return -1;
  }
  return usart_receive(instance, byte, 1U, timeout);
}

int usart_puts(USART_TypeDef *instance, const char *str, uint32_t timeout) {
  if ((instance == NULL) || (str == NULL)) {
    return -1;
  }

  while (*str != '\0') {
    /* Wait for TXE */
    uint32_t count = timeout;
    while (!instance->SR.bit.TXE) {
      if (timeout != 0U) {
        if (--count == 0U) {
          return -2;
        }
      }
    }

    instance->DR.reg = (uint32_t)(uint8_t)*str;
    str++;
  }

  /* Wait for TC after last character */
  {
    uint32_t count = timeout;
    while (!instance->SR.bit.TC) {
      if (timeout != 0U) {
        if (--count == 0U) {
          return -2;
        }
      }
    }
  }

  return 0;
}
