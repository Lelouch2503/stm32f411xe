/**
 * @file    stm32f411_gpio.h
 * @brief   GPIO driver for STM32F411xE — public API & STM32F411E-DISCO mappings
 *
 * Provides functions to:
 *  - Configure individual GPIO pins (mode, output type, speed, pull-up/down, alternate function)
 *  - Read and write pin digital states
 *  - Toggle pin output states
 *  - Read and write entire GPIO port values
 *  - Lock pin configuration until next system reset
 *  - Initialize user LEDs and button on STM32F411E-DISCO discovery board
 *
 * Reference: RM0383 Rev 3 — STM32F411xC/E Reference Manual, Chapter 8
 */

#ifndef STM32F411_GPIO_H
#define STM32F411_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f411_xe.h"

/* ══════════════════════════════════════════════════════════════════════
 * Enumerations — GPIO Configuration Options
 * ═════════════════════════════════════════════════════════════════════ */

/** GPIO Pin Number */
typedef enum {
  GPIO_PIN_0  = 0U,
  GPIO_PIN_1  = 1U,
  GPIO_PIN_2  = 2U,
  GPIO_PIN_3  = 3U,
  GPIO_PIN_4  = 4U,
  GPIO_PIN_5  = 5U,
  GPIO_PIN_6  = 6U,
  GPIO_PIN_7  = 7U,
  GPIO_PIN_8  = 8U,
  GPIO_PIN_9  = 9U,
  GPIO_PIN_10 = 10U,
  GPIO_PIN_11 = 11U,
  GPIO_PIN_12 = 12U,
  GPIO_PIN_13 = 13U,
  GPIO_PIN_14 = 14U,
  GPIO_PIN_15 = 15U,
  GPIO_PIN_ALL = 0xFFFFU
} GPIO_Pin_t;

/** GPIO Pin Mode (MODER[1:0]) */
typedef enum {
  GPIO_MODE_INPUT   = 0x0U, /**< Input floating / pull (reset state) */
  GPIO_MODE_OUTPUT  = 0x1U, /**< General purpose output mode        */
  GPIO_MODE_ALTFUNC = 0x2U, /**< Alternate function mode           */
  GPIO_MODE_ANALOG  = 0x3U  /**< Analog mode                       */
} GPIO_Mode_t;

/** GPIO Output Type (OTYPER) */
typedef enum {
  GPIO_OTYPE_PUSHPULL  = 0x0U, /**< Output push-pull (reset state)  */
  GPIO_OTYPE_OPENDRAIN = 0x1U  /**< Output open-drain               */
} GPIO_OType_t;

/** GPIO Output Speed (OSPEEDR[1:0]) */
typedef enum {
  GPIO_OSPEED_LOW    = 0x0U, /**< Low speed (2 MHz)                */
  GPIO_OSPEED_MEDIUM = 0x1U, /**< Medium speed (25 MHz)             */
  GPIO_OSPEED_FAST   = 0x2U, /**< Fast speed (50 MHz)               */
  GPIO_OSPEED_HIGH   = 0x3U  /**< High speed (100 MHz)              */
} GPIO_OSpeed_t;

/** GPIO Pull-up / Pull-down Resistors (PUPDR[1:0]) */
typedef enum {
  GPIO_PUPD_NONE     = 0x0U, /**< No pull-up, no pull-down          */
  GPIO_PUPD_PULLUP   = 0x1U, /**< Internal pull-up resistor        */
  GPIO_PUPD_PULLDOWN = 0x2U  /**< Internal pull-down resistor      */
} GPIO_PuPd_t;

/** GPIO Alternate Function Selection (AFR[3:0]) */
typedef enum {
  GPIO_AF0  = 0x0U,
  GPIO_AF1  = 0x1U,
  GPIO_AF2  = 0x2U,
  GPIO_AF3  = 0x3U,
  GPIO_AF4  = 0x4U,
  GPIO_AF5  = 0x5U,
  GPIO_AF6  = 0x6U,
  GPIO_AF7  = 0x7U,
  GPIO_AF8  = 0x8U,
  GPIO_AF9  = 0x9U,
  GPIO_AF10 = 0xAU,
  GPIO_AF11 = 0xBU,
  GPIO_AF12 = 0xCU,
  GPIO_AF13 = 0xDU,
  GPIO_AF14 = 0xEU,
  GPIO_AF15 = 0xFU,

  /* Descriptive Peripheral Aliases (STM32F411 RM0383 Table 9) */
  GPIO_AF0_SYSTEM     = 0x0U, /**< MCO, RTC_50Hz, TAMPER, SWJ/JTAG   */
  GPIO_AF1_TIM1_TIM2  = 0x1U, /**< TIM1, TIM2                        */
  GPIO_AF2_TIM3_TIM5  = 0x2U, /**< TIM3, TIM4, TIM5                  */
  GPIO_AF3_TIM9_TIM11 = 0x3U, /**< TIM9, TIM10, TIM11                */
  GPIO_AF4_I2C1_I2C3  = 0x4U, /**< I2C1, I2C2, I2C3                  */
  GPIO_AF5_SPI1_SPI5  = 0x5U, /**< SPI1, SPI2, SPI3, SPI4, SPI5      */
  GPIO_AF6_SPI3_SPI4  = 0x6U, /**< SPI3, SPI4                        */
  GPIO_AF7_USART1_2   = 0x7U, /**< USART1, USART2                    */
  GPIO_AF8_USART6     = 0x8U, /**< USART6                            */
  GPIO_AF9_I2C2_I2C3  = 0x9U, /**< I2C2, I2C3                        */
  GPIO_AF10_OTG_FS    = 0xAU, /**< USB OTG FS                        */
  GPIO_AF12_SDIO      = 0xCU, /**< SDIO                              */
  GPIO_AF15_EVENTOUT  = 0xFU  /**< EVENTOUT                          */
} GPIO_AltFunc_t;

/** GPIO Pin Logic Level State */
typedef enum {
  GPIO_PIN_RESET = 0U, /**< Logical Low  (0 V)  */
  GPIO_PIN_SET   = 1U  /**< Logical High (3.3 V) */
} GPIO_PinState_t;

/* ══════════════════════════════════════════════════════════════════════
 * Configuration Structures
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief Structure containing GPIO pin initialization parameters.
 */
typedef struct {
  GPIO_Pin_t     pin;       /**< GPIO pin number (GPIO_PIN_0 .. GPIO_PIN_15) */
  GPIO_Mode_t    mode;      /**< Operating mode                             */
  GPIO_OType_t   otype;     /**< Output type (Push-Pull or Open-Drain)      */
  GPIO_OSpeed_t  ospeed;    /**< Output slew rate / speed                   */
  GPIO_PuPd_t    pupd;      /**< Pull-up / pull-down resistor configuration  */
  GPIO_AltFunc_t alt_func;  /**< Alternate function (used when mode==ALTFUNC)*/
} GPIO_PinConfig_t;

/* ══════════════════════════════════════════════════════════════════════
 * STM32F411E-DISCO Board Mappings
 * ═════════════════════════════════════════════════════════════════════ */

/* User LEDs (GPIOD) */
#define DISCO_LED_GREEN_PIN   GPIO_PIN_12 /**< LD4 (Green)  */
#define DISCO_LED_ORANGE_PIN  GPIO_PIN_13 /**< LD3 (Orange) */
#define DISCO_LED_RED_PIN     GPIO_PIN_14 /**< LD5 (Red)    */
#define DISCO_LED_BLUE_PIN    GPIO_PIN_15 /**< LD6 (Blue)   */

#define DISCO_LED_PORT        GPIOD
#define DISCO_LED_RCC_EN      RCC_AHB1ENR_GPIODEN

/* User Push Button (GPIOA) */
#define DISCO_BTN_USER_PIN    GPIO_PIN_0  /**< B1 User Button */
#define DISCO_BTN_USER_PORT   GPIOA
#define DISCO_BTN_USER_RCC_EN RCC_AHB1ENR_GPIOAEN

/* ══════════════════════════════════════════════════════════════════════
 * Public API
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Initialize a GPIO pin according to the specified configuration.
 *
 * Configures AFR (if mode==ALTFUNC), OTYPER, OSPEEDR, PUPDR, and MODER
 * in the hardware-recommended safe sequence.
 *
 * @param  port    Pointer to target GPIO peripheral (GPIOA .. GPIOH).
 * @param  config  Pointer to pin configuration structure.
 * @retval  0   Success.
 * @retval -1   Invalid parameter (NULL pointer or out-of-range value).
 */
int gpio_init(GPIO_TypeDef *port, const GPIO_PinConfig_t *config);

/**
 * @brief  De-initialize a GPIO pin to its default reset state.
 *
 * Sets pin mode to input, speed to low, pull-none, push-pull, and clears AF.
 *
 * @param  port  Pointer to target GPIO peripheral (GPIOA .. GPIOH).
 * @param  pin   GPIO pin number to reset (GPIO_PIN_0 .. GPIO_PIN_15).
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 */
int gpio_deinit(GPIO_TypeDef *port, GPIO_Pin_t pin);

/**
 * @brief  Read the current input logical state of a GPIO pin.
 *
 * @param  port  Pointer to target GPIO peripheral (GPIOA .. GPIOH).
 * @param  pin   GPIO pin number to read (GPIO_PIN_0 .. GPIO_PIN_15).
 * @return Current pin state (GPIO_PIN_SET or GPIO_PIN_RESET).
 */
GPIO_PinState_t gpio_read_pin(const GPIO_TypeDef *port, GPIO_Pin_t pin);

/**
 * @brief  Write a logical state to a GPIO output pin.
 *
 * Uses atomic hardware Bit Set/Reset Register (BSRR).
 *
 * @param  port   Pointer to target GPIO peripheral (GPIOA .. GPIOH).
 * @param  pin    GPIO pin number to write (GPIO_PIN_0 .. GPIO_PIN_15).
 * @param  state  State to write (GPIO_PIN_SET or GPIO_PIN_RESET).
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 */
int gpio_write_pin(GPIO_TypeDef *port, GPIO_Pin_t pin, GPIO_PinState_t state);

/**
 * @brief  Toggle the logical state of a GPIO output pin.
 *
 * Reads current state from ODR and writes the inverted state to BSRR.
 *
 * @param  port  Pointer to target GPIO peripheral (GPIOA .. GPIOH).
 * @param  pin   GPIO pin number to toggle (GPIO_PIN_0 .. GPIO_PIN_15).
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 */
int gpio_toggle_pin(GPIO_TypeDef *port, GPIO_Pin_t pin);

/**
 * @brief  Read the full 16-bit input state of a GPIO port.
 *
 * @param  port  Pointer to target GPIO peripheral (GPIOA .. GPIOH).
 * @return 16-bit port input value. Returns 0 if port is NULL.
 */
uint16_t gpio_read_port(const GPIO_TypeDef *port);

/**
 * @brief  Write a 16-bit output value to a GPIO port.
 *
 * @param  port   Pointer to target GPIO peripheral (GPIOA .. GPIOH).
 * @param  value  16-bit output value to set on ODR.
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 */
int gpio_write_port(GPIO_TypeDef *port, uint16_t value);

/**
 * @brief  Lock a GPIO pin's configuration registers until the next MCU reset.
 *
 * Performs the hardware key write sequence on LCKR:
 * Write 1 -> Write 0 -> Write 1 -> Read 0 -> Read 1 (LCKK check).
 *
 * @param  port  Pointer to target GPIO peripheral (GPIOA .. GPIOH).
 * @param  pin   GPIO pin number to lock (GPIO_PIN_0 .. GPIO_PIN_15).
 * @retval  0   Success (pin locked).
 * @retval -1   Invalid parameter.
 * @retval -4   Lock failed / hardware error.
 */
int gpio_lock_pin(GPIO_TypeDef *port, GPIO_Pin_t pin);

/**
 * @brief  Convenience helper: Initialize all 4 user LEDs on STM32F411E-DISCO.
 *
 * Enables GPIOD RCC clock and configures PD12 (Green), PD13 (Orange),
 * PD14 (Red), PD15 (Blue) as Output Push-Pull Low Speed.
 *
 * @retval  0   Success.
 * @retval <0   Error code.
 */
int gpio_disco_leds_init(void);

/**
 * @brief  Convenience helper: Initialize User Push Button on STM32F411E-DISCO.
 *
 * Enables GPIOA RCC clock and configures PA0 as Input Floating / No-pull.
 *
 * @retval  0   Success.
 * @retval <0   Error code.
 */
int gpio_disco_button_init(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_GPIO_H */
