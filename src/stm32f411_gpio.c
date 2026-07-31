/**
 * @file    stm32f411_gpio.c
 * @brief   GPIO driver implementation for STM32F411xE & STM32F411E-DISCO
 *
 * Implements GPIO pin configuration, read/write/toggle, port access,
 * pin locking, and Discovery board LED & button initialization helpers.
 *
 * Reference: RM0383 Rev 3 — STM32F411xC/E Reference Manual, Chapter 8
 */

#include "stm32f411_gpio.h"
#include "stm32f411_rcc.h"
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════
 * Public API Implementation
 * ═════════════════════════════════════════════════════════════════════ */

int gpio_init(GPIO_TypeDef *port, const GPIO_PinConfig_t *config) {
  if ((port == NULL) || (config == NULL)) {
    return -1;
  }

  if (config->pin > GPIO_PIN_15) {
    return -1;
  }

  if ((config->mode > GPIO_MODE_ANALOG) ||
      (config->otype > GPIO_OTYPE_OPENDRAIN) ||
      (config->ospeed > GPIO_OSPEED_HIGH) ||
      (config->pupd > GPIO_PUPD_PULLDOWN) || (config->alt_func > GPIO_AF15)) {
    return -1;
  }

  uint32_t pin_num = (uint32_t)config->pin;
  uint32_t bit_pos2 = pin_num * 2U;

  /* 1. Alternate Function configuration (must be set before mode == ALTFUNC) */
  if (config->mode == GPIO_MODE_ALTFUNC) {
    uint32_t afr_idx = pin_num >> 3U; /* 0 for pins 0..7, 1 for pins 8..15 */
    uint32_t afr_shift = (pin_num & 0x7U) * 4U; /* 4 bits per pin */

    port->AFR[afr_idx] &= ~(0xFU << afr_shift);
    port->AFR[afr_idx] |= ((uint32_t)config->alt_func << afr_shift);
  }

  /* 2. Output Type configuration (OTYPER) */
  port->OTYPER &= ~(1U << pin_num);
  port->OTYPER |= ((uint32_t)config->otype << pin_num);

  /* 3. Output Speed configuration (OSPEEDR) */
  port->OSPEEDR &= ~(3U << bit_pos2);
  port->OSPEEDR |= ((uint32_t)config->ospeed << bit_pos2);

  /* 4. Pull-up / Pull-down configuration (PUPDR) */
  port->PUPDR &= ~(3U << bit_pos2);
  port->PUPDR |= ((uint32_t)config->pupd << bit_pos2);

  /* 5. Mode configuration (MODER) — set last for clean pin transition */
  port->MODER &= ~(3U << bit_pos2);
  port->MODER |= ((uint32_t)config->mode << bit_pos2);

  return 0;
}

int gpio_deinit(GPIO_TypeDef *port, GPIO_Pin_t pin) {
  if ((port == NULL) || (pin > GPIO_PIN_15)) {
    return -1;
  }

  uint32_t pin_num = (uint32_t)pin;
  uint32_t bit_pos2 = pin_num * 2U;
  uint32_t afr_idx = pin_num >> 3U;
  uint32_t afr_shift = (pin_num & 0x7U) * 4U;

  /* Reset pin to default: Input, Push-Pull, Low Speed, No Pull, AF0 */
  port->MODER &= ~(3U << bit_pos2);
  port->OTYPER &= ~(1U << pin_num);
  port->OSPEEDR &= ~(3U << bit_pos2);
  port->PUPDR &= ~(3U << bit_pos2);
  port->AFR[afr_idx] &= ~(0xFU << afr_shift);

  return 0;
}

GPIO_PinState_t gpio_read_pin(const GPIO_TypeDef *port, GPIO_Pin_t pin) {
  if ((port == NULL) || (pin > GPIO_PIN_15)) {
    return GPIO_PIN_RESET;
  }

  return ((port->IDR & (1U << (uint32_t)pin)) != 0U) ? GPIO_PIN_SET
                                                     : GPIO_PIN_RESET;
}

int gpio_write_pin(GPIO_TypeDef *port, GPIO_Pin_t pin, GPIO_PinState_t state) {
  if ((port == NULL) || (pin > GPIO_PIN_15)) {
    return -1;
  }

  if (state == GPIO_PIN_SET) {
    port->BSRR = (1U << (uint32_t)pin);
  } else {
    port->BSRR = (1U << ((uint32_t)pin + 16U));
  }

  return 0;
}

int gpio_toggle_pin(GPIO_TypeDef *port, GPIO_Pin_t pin) {
  if ((port == NULL) || (pin > GPIO_PIN_15)) {
    return -1;
  }

  uint32_t pin_mask = (1U << (uint32_t)pin);

  if ((port->ODR & pin_mask) != 0U) {
    port->BSRR = (1U << ((uint32_t)pin + 16U));
  } else {
    port->BSRR = pin_mask;
  }

  return 0;
}

uint16_t gpio_read_port(const GPIO_TypeDef *port) {
  if (port == NULL) {
    return 0U;
  }

  return (uint16_t)(port->IDR & 0xFFFFU);
}

int gpio_write_port(GPIO_TypeDef *port, uint16_t value) {
  if (port == NULL) {
    return -1;
  }

  port->ODR = (uint32_t)value;
  return 0;
}

int gpio_lock_pin(GPIO_TypeDef *port, GPIO_Pin_t pin) {
  if ((port == NULL) || (pin > GPIO_PIN_15)) {
    return -1;
  }

  uint32_t pin_mask = (1U << (uint32_t)pin);
  uint32_t lock_val = GPIO_LCKR_LCKK | pin_mask;

  /* Key write sequence (RM0383 §8.4.8):
   * Write 1 -> Write 0 -> Write 1 -> Read 0 -> Read 1 */
  port->LCKR = lock_val;
  port->LCKR = pin_mask;
  port->LCKR = lock_val;

  (void)port->LCKR;
  uint32_t read_back = port->LCKR;

  if ((read_back & GPIO_LCKR_LCKK) != 0U) {
    return 0; /* Lock active */
  }

  return -4; /* Lock sequence failed */
}

/* ══════════════════════════════════════════════════════════════════════
 * STM32F411E-DISCO Convenience Helpers
 * ═════════════════════════════════════════════════════════════════════ */

int gpio_disco_leds_init(void) {
  /* 1. Enable GPIOD clock via RCC driver */
  rcc_ahb1_clk_enable(DISCO_LED_RCC_EN);

  /* 2. Configure PD12 (Green), PD13 (Orange), PD14 (Red), PD15 (Blue) */
  const GPIO_Pin_t leds[4] = {DISCO_LED_GREEN_PIN, DISCO_LED_ORANGE_PIN,
                              DISCO_LED_RED_PIN, DISCO_LED_BLUE_PIN};

  for (size_t i = 0U; i < 4U; i++) {
    GPIO_PinConfig_t cfg = {.pin = leds[i],
                            .mode = GPIO_MODE_OUTPUT,
                            .otype = GPIO_OTYPE_PUSHPULL,
                            .ospeed = GPIO_OSPEED_LOW,
                            .pupd = GPIO_PUPD_NONE,
                            .alt_func = GPIO_AF0};

    int status = gpio_init(DISCO_LED_PORT, &cfg);
    if (status != 0) {
      return status;
    }
  }

  return 0;
}

int gpio_disco_button_init(void) {
  /* 1. Enable GPIOA clock via RCC driver */
  rcc_ahb1_clk_enable(DISCO_BTN_USER_RCC_EN);

  /* 2. Configure PA0 as Input (User Button has hardware external pull-down on
   * Discovery board) */
  GPIO_PinConfig_t cfg = {.pin = DISCO_BTN_USER_PIN,
                          .mode = GPIO_MODE_INPUT,
                          .otype = GPIO_OTYPE_PUSHPULL,
                          .ospeed = GPIO_OSPEED_LOW,
                          .pupd = GPIO_PUPD_NONE,
                          .alt_func = GPIO_AF0};

  return gpio_init(DISCO_BTN_USER_PORT, &cfg);
}
