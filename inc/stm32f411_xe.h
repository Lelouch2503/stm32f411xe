/**
 * @file    stm32f411_xe.h
 * @brief   STM32F411xE device-level register definitions (bare-metal)
 *
 * Provides:
 *  - IO access qualifiers (__IO, __I, __O)
 *  - Memory-map base addresses
 *  - Peripheral register structures (RCC, FLASH, PWR)
 *  - Bit-field position/mask macros for all RCC-related registers
 *
 * Reference: RM0383 Rev 3 — STM32F411xC/E Reference Manual
 */

#ifndef STM32F411_XE_H
#define STM32F411_XE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ══════════════════════════════════════════════════════════════════════
 * IO Access Qualifiers
 * ═════════════════════════════════════════════════════════════════════ */
#define __IO volatile      /**< Read / Write access            */
#define __I volatile const /**< Read-only access               */
#define __O volatile       /**< Write-only access              */

/* ══════════════════════════════════════════════════════════════════════
 * Clock Source Default Values
 * ═════════════════════════════════════════════════════════════════════ */
#ifndef HSE_VALUE
#define HSE_VALUE 8000000U /**< External crystal: 8 MHz (Discovery board) */
#endif

#ifndef HSI_VALUE
#define HSI_VALUE 16000000U /**< Internal RC oscillator: 16 MHz */
#endif

/* ══════════════════════════════════════════════════════════════════════
 * Memory Map — Base Addresses
 * ═════════════════════════════════════════════════════════════════════ */
#define PERIPH_BASE 0x40000000UL

#define APB1PERIPH_BASE (PERIPH_BASE)
#define APB2PERIPH_BASE (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE (PERIPH_BASE + 0x10000000UL)

/* ── Peripheral Base Addresses ─────────────────────────────────────── */
#define PWR_BASE    (APB1PERIPH_BASE + 0x7000UL)
#define RCC_BASE    (AHB1PERIPH_BASE + 0x3800UL)
#define FLASH_R_BASE (AHB1PERIPH_BASE + 0x3C00UL)

/* USART base addresses (RM0383 §2.3, Table 1) */
#define USART2_BASE (APB1PERIPH_BASE + 0x4400UL) /**< APB1 */
#define USART1_BASE (APB2PERIPH_BASE + 0x1000UL) /**< APB2 */
#define USART6_BASE (APB2PERIPH_BASE + 0x1400UL) /**< APB2 */

/* I2C base addresses (RM0383 §2.3, Table 1) */
#define I2C1_BASE (APB1PERIPH_BASE + 0x5400UL) /**< APB1 */
#define I2C2_BASE (APB1PERIPH_BASE + 0x5800UL) /**< APB1 */
#define I2C3_BASE (APB1PERIPH_BASE + 0x5C00UL) /**< APB1 */

/* SPI/I2S base addresses (RM0383, Table 1) */
#define SPI1_BASE (APB2PERIPH_BASE + 0x3000UL) /**< APB2 */
#define SPI2_BASE (APB1PERIPH_BASE + 0x3800UL) /**< APB1 */
#define SPI3_BASE (APB1PERIPH_BASE + 0x3C00UL) /**< APB1 */
#define SPI4_BASE (APB2PERIPH_BASE + 0x3400UL) /**< APB2 */
#define SPI5_BASE (APB2PERIPH_BASE + 0x5000UL) /**< APB2 */

/* GPIO base addresses */
#define GPIOA_BASE  (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE  (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE  (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE  (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE  (AHB1PERIPH_BASE + 0x1000UL)
#define GPIOH_BASE  (AHB1PERIPH_BASE + 0x1C00UL)

/* ══════════════════════════════════════════════════════════════════════
 * Peripheral Register Structures
 * ═════════════════════════════════════════════════════════════════════ */

/* ── RCC (Reset and Clock Control) ─────────────────────────────────── */
typedef struct {
  union {
    __IO uint32_t reg;         /**< 0x00 Clock control register */
    struct {
      __IO uint32_t HSION      : 1;  /**< [0] Internal high-speed clock enable */
      __I  uint32_t HSIRDY     : 1;  /**< [1] Internal high-speed clock ready flag */
      uint32_t                 : 1;  /**< [2] Reserved */
      __IO uint32_t HSITRIM    : 5;  /**< [7:3] Internal high-speed clock trimming */
      __I  uint32_t HSICAL     : 8;  /**< [15:8] Internal high-speed clock calibration */
      __IO uint32_t HSEON      : 1;  /**< [16] External high-speed clock enable */
      __I  uint32_t HSERDY     : 1;  /**< [17] External high-speed clock ready flag */
      __IO uint32_t HSEBYP     : 1;  /**< [18] External high-speed clock bypass */
      __IO uint32_t CSSON      : 1;  /**< [19] Clock security system enable */
      uint32_t                 : 4;  /**< [23:20] Reserved */
      __IO uint32_t PLLON      : 1;  /**< [24] Main PLL enable */
      __I  uint32_t PLLRDY     : 1;  /**< [25] Main PLL clock ready flag */
      __IO uint32_t PLLI2SON   : 1;  /**< [26] PLLI2S enable */
      __I  uint32_t PLLI2SRDY  : 1;  /**< [27] PLLI2S clock ready flag */
      uint32_t                 : 4;  /**< [31:28] Reserved */
    } bit;
  } CR;

  union {
    __IO uint32_t reg;    /**< 0x04 PLL configuration register */
    struct {
      __IO uint32_t PLLM       : 6;  /**< [5:0] Division factor for the main PLL and PLLI2S input clock */
      __IO uint32_t PLLN       : 9;  /**< [14:6] Main PLL VCO multiplier */
      uint32_t                 : 1;  /**< [15] Reserved */
      __IO uint32_t PLLP       : 2;  /**< [17:16] Main PLL division factor for main system clock */
      uint32_t                 : 4;  /**< [21:18] Reserved */
      __IO uint32_t PLLSRC     : 1;  /**< [22] Main PLL and PLLI2S entry clock source (0=HSI, 1=HSE) */
      uint32_t                 : 1;  /**< [23] Reserved */
      __IO uint32_t PLLQ       : 4;  /**< [27:24] Main PLL division factor for USB OTG FS, SDIO, etc. */
      uint32_t                 : 4;  /**< [31:28] Reserved */
    } bit;
  } PLLCFGR;

  union {
    __IO uint32_t reg;       /**< 0x08 Clock configuration register */
    struct {
      __IO uint32_t SW         : 2;  /**< [1:0] System clock switch */
      __I  uint32_t SWS        : 2;  /**< [3:2] System clock switch status */
      __IO uint32_t HPRE       : 4;  /**< [7:4] AHB prescaler */
      uint32_t                 : 2;  /**< [9:8] Reserved */
      __IO uint32_t PPRE1      : 3;  /**< [12:10] APB Low speed prescaler (APB1) */
      __IO uint32_t PPRE2      : 3;  /**< [15:13] APB High speed prescaler (APB2) */
      __IO uint32_t RTCPRE     : 5;  /**< [20:16] HSE division factor for RTC clock */
      __IO uint32_t MCO1       : 2;  /**< [22:21] Microcontroller clock output 1 */
      __IO uint32_t I2SSRC     : 1;  /**< [23] I2S clock source selection */
      __IO uint32_t MCO1PRE    : 3;  /**< [26:24] MCO1 prescaler */
      __IO uint32_t MCO2PRE    : 3;  /**< [29:27] MCO2 prescaler */
      __IO uint32_t MCO2       : 2;  /**< [31:30] Microcontroller clock output 2 */
    } bit;
  } CFGR;

  union {
    __IO uint32_t reg;        /**< 0x0C Clock interrupt register */
    struct {
      __I  uint32_t LSIRDYF    : 1;  /**< [0] LSI ready interrupt flag */
      __I  uint32_t LSERDYF    : 1;  /**< [1] LSE ready interrupt flag */
      __I  uint32_t HSIRDYF    : 1;  /**< [2] HSI ready interrupt flag */
      __I  uint32_t HSERDYF    : 1;  /**< [3] HSE ready interrupt flag */
      __I  uint32_t PLLRDYF    : 1;  /**< [4] PLL ready interrupt flag */
      __I  uint32_t PLLI2SRDYF : 1;  /**< [5] PLLI2S ready interrupt flag */
      uint32_t                 : 1;  /**< [6] Reserved */
      __I  uint32_t CSSF       : 1;  /**< [7] Clock security system interrupt flag */
      __IO uint32_t LSIRDYIE   : 1;  /**< [8] LSI ready interrupt enable */
      __IO uint32_t LSERDYIE   : 1;  /**< [9] LSE ready interrupt enable */
      __IO uint32_t HSIRDYIE   : 1;  /**< [10] HSI ready interrupt enable */
      __IO uint32_t HSERDYIE   : 1;  /**< [11] HSE ready interrupt enable */
      __IO uint32_t PLLRDYIE   : 1;  /**< [12] PLL ready interrupt enable */
      __IO uint32_t PLLI2SRDYIE: 1;  /**< [13] PLLI2S ready interrupt enable */
      uint32_t                 : 2;  /**< [15:14] Reserved */
      __O  uint32_t LSIRDYC    : 1;  /**< [16] LSI ready interrupt clear */
      __O  uint32_t LSERDYC    : 1;  /**< [17] LSE ready interrupt clear */
      __O  uint32_t HSIRDYC    : 1;  /**< [18] HSI ready interrupt clear */
      __O  uint32_t HSERDYC    : 1;  /**< [19] HSE ready interrupt clear */
      __O  uint32_t PLLRDYC    : 1;  /**< [20] PLL ready interrupt clear */
      __O  uint32_t PLLI2SRDYC : 1;  /**< [21] PLLI2S ready interrupt clear */
      uint32_t                 : 1;  /**< [22] Reserved */
      __O  uint32_t CSSC       : 1;  /**< [23] Clock security system interrupt clear */
      uint32_t                 : 8;  /**< [31:24] Reserved */
    } bit;
  } CIR;

  union {
    __IO uint32_t reg;   /**< 0x10 AHB1 peripheral reset register */
    struct {
      __IO uint32_t GPIOARST   : 1;  /**< [0] GPIOA reset */
      __IO uint32_t GPIOBRST   : 1;  /**< [1] GPIOB reset */
      __IO uint32_t GPIOCRST   : 1;  /**< [2] GPIOC reset */
      __IO uint32_t GPIODRST   : 1;  /**< [3] GPIOD reset */
      __IO uint32_t GPIOERST   : 1;  /**< [4] GPIOE reset */
      uint32_t                 : 2;  /**< [6:5] Reserved */
      __IO uint32_t GPIOHRST   : 1;  /**< [7] GPIOH reset */
      uint32_t                 : 4;  /**< [11:8] Reserved */
      __IO uint32_t CRCRST     : 1;  /**< [12] CRC reset */
      uint32_t                 : 8;  /**< [20:13] Reserved */
      __IO uint32_t DMA1RST    : 1;  /**< [21] DMA1 reset */
      __IO uint32_t DMA2RST    : 1;  /**< [22] DMA2 reset */
      uint32_t                 : 9;  /**< [31:23] Reserved */
    } bit;
  } AHB1RSTR;

  union {
    __IO uint32_t reg;   /**< 0x14 AHB2 peripheral reset register */
    struct {
      uint32_t                 : 7;  /**< [6:0] Reserved */
      __IO uint32_t OTGFSRST   : 1;  /**< [7] USB OTG FS reset */
      uint32_t                 : 24; /**< [31:8] Reserved */
    } bit;
  } AHB2RSTR;

  uint32_t RESERVED0[2];    /**< 0x18–0x1C Reserved */

  union {
    __IO uint32_t reg;   /**< 0x20 APB1 peripheral reset register */
    struct {
      __IO uint32_t TIM2RST    : 1;  /**< [0] TIM2 reset */
      __IO uint32_t TIM3RST    : 1;  /**< [1] TIM3 reset */
      __IO uint32_t TIM4RST    : 1;  /**< [2] TIM4 reset */
      __IO uint32_t TIM5RST    : 1;  /**< [3] TIM5 reset */
      uint32_t                 : 7;  /**< [10:4] Reserved */
      __IO uint32_t WWDGRST    : 1;  /**< [11] Window watchdog reset */
      uint32_t                 : 2;  /**< [13:12] Reserved */
      __IO uint32_t SPI2RST    : 1;  /**< [14] SPI2 reset */
      __IO uint32_t SPI3RST    : 1;  /**< [15] SPI3 reset */
      uint32_t                 : 1;  /**< [16] Reserved */
      __IO uint32_t USART2RST  : 1;  /**< [17] USART2 reset */
      uint32_t                 : 3;  /**< [20:18] Reserved */
      __IO uint32_t I2C1RST    : 1;  /**< [21] I2C1 reset */
      __IO uint32_t I2C2RST    : 1;  /**< [22] I2C2 reset */
      __IO uint32_t I2C3RST    : 1;  /**< [23] I2C3 reset */
      uint32_t                 : 4;  /**< [27:24] Reserved */
      __IO uint32_t PWRRST     : 1;  /**< [28] Power interface reset */
      uint32_t                 : 3;  /**< [31:29] Reserved */
    } bit;
  } APB1RSTR;

  union {
    __IO uint32_t reg;   /**< 0x24 APB2 peripheral reset register */
    struct {
      __IO uint32_t TIM1RST    : 1;  /**< [0] TIM1 reset */
      uint32_t                 : 3;  /**< [3:1] Reserved */
      __IO uint32_t USART1RST  : 1;  /**< [4] USART1 reset */
      __IO uint32_t USART6RST  : 1;  /**< [5] USART6 reset */
      uint32_t                 : 2;  /**< [7:6] Reserved */
      __IO uint32_t ADC1RST    : 1;  /**< [8] ADC1 reset */
      uint32_t                 : 2;  /**< [10:9] Reserved */
      __IO uint32_t SDIORST    : 1;  /**< [11] SDIO reset */
      __IO uint32_t SPI1RST    : 1;  /**< [12] SPI1 reset */
      __IO uint32_t SPI4RST    : 1;  /**< [13] SPI4 reset */
      __IO uint32_t SYSCFGRST  : 1;  /**< [14] System configuration controller reset */
      uint32_t                 : 1;  /**< [15] Reserved */
      __IO uint32_t TIM9RST    : 1;  /**< [16] TIM9 reset */
      __IO uint32_t TIM10RST   : 1;  /**< [17] TIM10 reset */
      __IO uint32_t TIM11RST   : 1;  /**< [18] TIM11 reset */
      uint32_t                 : 1;  /**< [19] Reserved */
      __IO uint32_t SPI5RST    : 1;  /**< [20] SPI5 reset */
      uint32_t                 : 11; /**< [31:21] Reserved */
    } bit;
  } APB2RSTR;

  uint32_t RESERVED1[2];    /**< 0x28–0x2C Reserved */

  union {
    __IO uint32_t reg;    /**< 0x30 AHB1 peripheral clock enable register */
    struct {
      __IO uint32_t GPIOAEN    : 1;  /**< [0] GPIOA clock enable */
      __IO uint32_t GPIOBEN    : 1;  /**< [1] GPIOB clock enable */
      __IO uint32_t GPIOCEN    : 1;  /**< [2] GPIOC clock enable */
      __IO uint32_t GPIODEN    : 1;  /**< [3] GPIOD clock enable */
      __IO uint32_t GPIOEEN    : 1;  /**< [4] GPIOE clock enable */
      uint32_t                 : 2;  /**< [6:5] Reserved */
      __IO uint32_t GPIOHEN    : 1;  /**< [7] GPIOH clock enable */
      uint32_t                 : 4;  /**< [11:8] Reserved */
      __IO uint32_t CRCEN      : 1;  /**< [12] CRC clock enable */
      uint32_t                 : 8;  /**< [20:13] Reserved */
      __IO uint32_t DMA1EN     : 1;  /**< [21] DMA1 clock enable */
      __IO uint32_t DMA2EN     : 1;  /**< [22] DMA2 clock enable */
      uint32_t                 : 9;  /**< [31:23] Reserved */
    } bit;
  } AHB1ENR;

  union {
    __IO uint32_t reg;    /**< 0x34 AHB2 peripheral clock enable register */
    struct {
      uint32_t                 : 7;  /**< [6:0] Reserved */
      __IO uint32_t OTGFSEN    : 1;  /**< [7] USB OTG FS clock enable */
      uint32_t                 : 24; /**< [31:8] Reserved */
    } bit;
  } AHB2ENR;

  uint32_t RESERVED2[2];    /**< 0x38–0x3C Reserved */

  union {
    __IO uint32_t reg;    /**< 0x40 APB1 peripheral clock enable register */
    struct {
      __IO uint32_t TIM2EN     : 1;  /**< [0] TIM2 clock enable */
      __IO uint32_t TIM3EN     : 1;  /**< [1] TIM3 clock enable */
      __IO uint32_t TIM4EN     : 1;  /**< [2] TIM4 clock enable */
      __IO uint32_t TIM5EN     : 1;  /**< [3] TIM5 clock enable */
      uint32_t                 : 7;  /**< [10:4] Reserved */
      __IO uint32_t WWDGEN     : 1;  /**< [11] Window watchdog clock enable */
      uint32_t                 : 2;  /**< [13:12] Reserved */
      __IO uint32_t SPI2EN     : 1;  /**< [14] SPI2 clock enable */
      __IO uint32_t SPI3EN     : 1;  /**< [15] SPI3 clock enable */
      uint32_t                 : 1;  /**< [16] Reserved */
      __IO uint32_t USART2EN   : 1;  /**< [17] USART2 clock enable */
      uint32_t                 : 3;  /**< [20:18] Reserved */
      __IO uint32_t I2C1EN     : 1;  /**< [21] I2C1 clock enable */
      __IO uint32_t I2C2EN     : 1;  /**< [22] I2C2 clock enable */
      __IO uint32_t I2C3EN     : 1;  /**< [23] I2C3 clock enable */
      uint32_t                 : 4;  /**< [27:24] Reserved */
      __IO uint32_t PWREN      : 1;  /**< [28] Power interface clock enable */
      uint32_t                 : 3;  /**< [31:29] Reserved */
    } bit;
  } APB1ENR;

  union {
    __IO uint32_t reg;    /**< 0x44 APB2 peripheral clock enable register */
    struct {
      __IO uint32_t TIM1EN     : 1;  /**< [0] TIM1 clock enable */
      uint32_t                 : 3;  /**< [3:1] Reserved */
      __IO uint32_t USART1EN   : 1;  /**< [4] USART1 clock enable */
      __IO uint32_t USART6EN   : 1;  /**< [5] USART6 clock enable */
      uint32_t                 : 2;  /**< [7:6] Reserved */
      __IO uint32_t ADC1EN     : 1;  /**< [8] ADC1 clock enable */
      uint32_t                 : 2;  /**< [10:9] Reserved */
      __IO uint32_t SDIOEN     : 1;  /**< [11] SDIO clock enable */
      __IO uint32_t SPI1EN     : 1;  /**< [12] SPI1 clock enable */
      __IO uint32_t SPI4EN     : 1;  /**< [13] SPI4 clock enable */
      __IO uint32_t SYSCFGEN   : 1;  /**< [14] System configuration controller clock enable */
      uint32_t                 : 1;  /**< [15] Reserved */
      __IO uint32_t TIM9EN     : 1;  /**< [16] TIM9 clock enable */
      __IO uint32_t TIM10EN    : 1;  /**< [17] TIM10 clock enable */
      __IO uint32_t TIM11EN    : 1;  /**< [18] TIM11 clock enable */
      uint32_t                 : 1;  /**< [19] Reserved */
      __IO uint32_t SPI5EN     : 1;  /**< [20] SPI5 clock enable */
      uint32_t                 : 11; /**< [31:21] Reserved */
    } bit;
  } APB2ENR;

  uint32_t RESERVED3[2];    /**< 0x48–0x4C Reserved */

  union {
    __IO uint32_t reg;  /**< 0x50 AHB1 clock enable in low-power register */
    struct {
      __IO uint32_t GPIOALPEN  : 1;  /**< [0] GPIOA clock enable in low-power */
      __IO uint32_t GPIOBLPEN  : 1;  /**< [1] GPIOB clock enable in low-power */
      __IO uint32_t GPIOCLPEN  : 1;  /**< [2] GPIOC clock enable in low-power */
      __IO uint32_t GPIODLPEN  : 1;  /**< [3] GPIOD clock enable in low-power */
      __IO uint32_t GPIOELPEN  : 1;  /**< [4] GPIOE clock enable in low-power */
      uint32_t                 : 2;  /**< [6:5] Reserved */
      __IO uint32_t GPIOHLPEN  : 1;  /**< [7] GPIOH clock enable in low-power */
      uint32_t                 : 4;  /**< [11:8] Reserved */
      __IO uint32_t CRCLPEN    : 1;  /**< [12] CRC clock enable in low-power */
      uint32_t                 : 2;  /**< [14:13] Reserved */
      __IO uint32_t FLASHLPEN  : 1;  /**< [15] Flash interface clock enable in low-power */
      __IO uint32_t SRAM1LPEN  : 1;  /**< [16] SRAM1 clock enable in low-power */
      uint32_t                 : 4;  /**< [20:17] Reserved */
      __IO uint32_t DMA1LPEN   : 1;  /**< [21] DMA1 clock enable in low-power */
      __IO uint32_t DMA2LPEN   : 1;  /**< [22] DMA2 clock enable in low-power */
      uint32_t                 : 9;  /**< [31:23] Reserved */
    } bit;
  } AHB1LPENR;

  union {
    __IO uint32_t reg;  /**< 0x54 AHB2 clock enable in low-power register */
    struct {
      uint32_t                 : 7;  /**< [6:0] Reserved */
      __IO uint32_t OTGFSLPEN  : 1;  /**< [7] USB OTG FS clock enable in low-power */
      uint32_t                 : 24; /**< [31:8] Reserved */
    } bit;
  } AHB2LPENR;

  uint32_t RESERVED4[2];    /**< 0x58–0x5C Reserved */

  union {
    __IO uint32_t reg;  /**< 0x60 APB1 clock enable in low-power register */
    struct {
      __IO uint32_t TIM2LPEN   : 1;  /**< [0] TIM2 clock enable in low-power */
      __IO uint32_t TIM3LPEN   : 1;  /**< [1] TIM3 clock enable in low-power */
      __IO uint32_t TIM4LPEN   : 1;  /**< [2] TIM4 clock enable in low-power */
      __IO uint32_t TIM5LPEN   : 1;  /**< [3] TIM5 clock enable in low-power */
      uint32_t                 : 7;  /**< [10:4] Reserved */
      __IO uint32_t WWDGLPEN   : 1;  /**< [11] Window watchdog clock enable in low-power */
      uint32_t                 : 2;  /**< [13:12] Reserved */
      __IO uint32_t SPI2LPEN   : 1;  /**< [14] SPI2 clock enable in low-power */
      __IO uint32_t SPI3LPEN   : 1;  /**< [15] SPI3 clock enable in low-power */
      uint32_t                 : 1;  /**< [16] Reserved */
      __IO uint32_t USART2LPEN : 1;  /**< [17] USART2 clock enable in low-power */
      uint32_t                 : 3;  /**< [20:18] Reserved */
      __IO uint32_t I2C1LPEN   : 1;  /**< [21] I2C1 clock enable in low-power */
      __IO uint32_t I2C2LPEN   : 1;  /**< [22] I2C2 clock enable in low-power */
      __IO uint32_t I2C3LPEN   : 1;  /**< [23] I2C3 clock enable in low-power */
      uint32_t                 : 4;  /**< [27:24] Reserved */
      __IO uint32_t PWRLPEN    : 1;  /**< [28] Power interface clock enable in low-power */
      uint32_t                 : 3;  /**< [31:29] Reserved */
    } bit;
  } APB1LPENR;

  union {
    __IO uint32_t reg;  /**< 0x64 APB2 clock enable in low-power register */
    struct {
      __IO uint32_t TIM1LPEN   : 1;  /**< [0] TIM1 clock enable in low-power */
      uint32_t                 : 3;  /**< [3:1] Reserved */
      __IO uint32_t USART1LPEN : 1;  /**< [4] USART1 clock enable in low-power */
      __IO uint32_t USART6LPEN : 1;  /**< [5] USART6 clock enable in low-power */
      uint32_t                 : 2;  /**< [7:6] Reserved */
      __IO uint32_t ADC1LPEN   : 1;  /**< [8] ADC1 clock enable in low-power */
      uint32_t                 : 2;  /**< [10:9] Reserved */
      __IO uint32_t SDIOLPEN   : 1;  /**< [11] SDIO clock enable in low-power */
      __IO uint32_t SPI1LPEN   : 1;  /**< [12] SPI1 clock enable in low-power */
      __IO uint32_t SPI4LPEN   : 1;  /**< [13] SPI4 clock enable in low-power */
      __IO uint32_t SYSCFGLPEN : 1;  /**< [14] System configuration controller clock enable in low-power */
      uint32_t                 : 1;  /**< [15] Reserved */
      __IO uint32_t TIM9LPEN   : 1;  /**< [16] TIM9 clock enable in low-power */
      __IO uint32_t TIM10LPEN  : 1;  /**< [17] TIM10 clock enable in low-power */
      __IO uint32_t TIM11LPEN  : 1;  /**< [18] TIM11 clock enable in low-power */
      uint32_t                 : 1;  /**< [19] Reserved */
      __IO uint32_t SPI5LPEN   : 1;  /**< [20] SPI5 clock enable in low-power */
      uint32_t                 : 11; /**< [31:21] Reserved */
    } bit;
  } APB2LPENR;

  uint32_t RESERVED5[2];    /**< 0x68–0x6C Reserved */

  union {
    __IO uint32_t reg;       /**< 0x70 Backup domain control register */
    struct {
      __IO uint32_t LSEON      : 1;  /**< [0] External low-speed oscillator enable */
      __I  uint32_t LSERDY     : 1;  /**< [1] External low-speed oscillator ready */
      __IO uint32_t LSEBYP     : 1;  /**< [2] External low-speed oscillator bypass */
      uint32_t                 : 5;  /**< [7:3] Reserved */
      __IO uint32_t RTCSEL     : 2;  /**< [9:8] RTC clock source selection */
      uint32_t                 : 5;  /**< [14:10] Reserved */
      __IO uint32_t RTCEN      : 1;  /**< [15] RTC clock enable */
      __IO uint32_t BDRST      : 1;  /**< [16] Backup domain software reset */
      uint32_t                 : 15; /**< [31:17] Reserved */
    } bit;
  } BDCR;

  union {
    __IO uint32_t reg;        /**< 0x74 Clock control & status register */
    struct {
      __IO uint32_t LSION      : 1;  /**< [0] Internal low-speed oscillator enable */
      __I  uint32_t LSIRDY     : 1;  /**< [1] Internal low-speed oscillator ready */
      uint32_t                 : 22; /**< [23:2] Reserved */
      __IO uint32_t RMVF       : 1;  /**< [24] Remove reset flag */
      __IO uint32_t BORRSTF    : 1;  /**< [25] BOR reset flag */
      __IO uint32_t PINRSTF    : 1;  /**< [26] PIN reset flag */
      __IO uint32_t PORRSTF    : 1;  /**< [27] POR/PDR reset flag */
      __IO uint32_t SFTRSTF    : 1;  /**< [28] Software reset flag */
      __IO uint32_t IWDGRSTF   : 1;  /**< [29] Independent watchdog reset flag */
      __IO uint32_t WWDGRSTF   : 1;  /**< [30] Window watchdog reset flag */
      __IO uint32_t LPWRRSTF   : 1;  /**< [31] Low-power reset flag */
    } bit;
  } CSR;

  uint32_t RESERVED6[2];    /**< 0x78–0x7C Reserved */

  union {
    __IO uint32_t reg;      /**< 0x80 Spread spectrum clock generation register */
    struct {
      __IO uint32_t MODPER     : 13; /**< [12:0] Modulation period */
      __IO uint32_t INCSTEP    : 12; /**< [24:13] Incremental step */
      uint32_t                 : 5;  /**< [29:25] Reserved */
      __IO uint32_t SPREADSEL  : 1;  /**< [30] Spread Select */
      __IO uint32_t SSCGEN     : 1;  /**< [31] Spread spectrum modulation enable */
    } bit;
  } SSCGR;

  union {
    __IO uint32_t reg; /**< 0x84 PLLI2S configuration register */
    struct {
      uint32_t                 : 6;  /**< [5:0] Reserved */
      __IO uint32_t PLLI2SN    : 9;  /**< [14:6] PLLI2S VCO multiplier */
      uint32_t                 : 13; /**< [27:15] Reserved */
      __IO uint32_t PLLI2SR    : 3;  /**< [30:28] PLLI2S division factor for I2S clock */
      uint32_t                 : 1;  /**< [31] Reserved */
    } bit;
  } PLLI2SCFGR;

  uint32_t RESERVED7;       /**< 0x88 Reserved */

  union {
    __IO uint32_t reg;    /**< 0x8C Dedicated clocks configuration register */
    struct {
      uint32_t                 : 24; /**< [23:0] Reserved */
      __IO uint32_t CKOUTSRC   : 2;  /**< [25:24] MCO2/MCO1 output clock source selection */
      uint32_t                 : 6;  /**< [31:26] Reserved */
    } bit;
  } DCKCFGR;
} RCC_TypeDef;

/* ── FLASH Interface ───────────────────────────────────────────────── */
typedef struct {
  union {
    __IO uint32_t reg;     /**< 0x00  Access control          */
    struct {
      __IO uint32_t LATENCY  : 4;  /**< [3:0] Latency wait states */
      uint32_t               : 4;  /**< [7:4] Reserved */
      __IO uint32_t PRFTEN   : 1;  /**< [8] Prefetch enable */
      __IO uint32_t ICEN     : 1;  /**< [9] Instruction cache enable */
      __IO uint32_t DCEN     : 1;  /**< [10] Data cache enable */
      __IO uint32_t ICRST    : 1;  /**< [11] Instruction cache reset */
      __IO uint32_t DCRST    : 1;  /**< [12] Data cache reset */
      uint32_t               : 19; /**< [31:13] Reserved */
    } bit;
  } ACR;
  __IO uint32_t KEYR;    /**< 0x04  Key                     */
  __IO uint32_t OPTKEYR; /**< 0x08  Option key              */
  union {
    __IO uint32_t reg;      /**< 0x0C  Status                  */
    struct {
      __IO uint32_t EOP      : 1;  /**< [0] End of operation */
      __IO uint32_t OPERR    : 1;  /**< [1] Operation error */
      uint32_t               : 2;  /**< [3:2] Reserved */
      __IO uint32_t WRPERR   : 1;  /**< [4] Write protection error */
      __IO uint32_t PGAERR   : 1;  /**< [5] Programming alignment error */
      __IO uint32_t PGPERR   : 1;  /**< [6] Programming parallelism error */
      __IO uint32_t PGSERR   : 1;  /**< [7] Programming sequence error */
      uint32_t               : 8;  /**< [15:8] Reserved */
      __I  uint32_t BSY      : 1;  /**< [16] Busy */
      uint32_t               : 15; /**< [31:17] Reserved */
    } bit;
  } SR;
  union {
    __IO uint32_t reg;      /**< 0x10  Control                 */
    struct {
      __IO uint32_t PG       : 1;  /**< [0] Programming */
      __IO uint32_t SER      : 1;  /**< [1] Sector Erase */
      __IO uint32_t MER      : 1;  /**< [2] Mass Erase */
      __IO uint32_t SNB      : 4;  /**< [6:3] Sector number */
      uint32_t               : 1;  /**< [7] Reserved */
      __IO uint32_t PSIZE    : 2;  /**< [9:8] Program size */
      uint32_t               : 6;  /**< [15:10] Reserved */
      __IO uint32_t STRT     : 1;  /**< [16] Start */
      uint32_t               : 8;  /**< [24:17] Reserved */
      __IO uint32_t EOPIE    : 1;  /**< [25] End of operation interrupt enable */
      __IO uint32_t ERRIE    : 1;  /**< [26] Error interrupt enable */
      uint32_t               : 4;  /**< [30:27] Reserved */
      __IO uint32_t LOCK     : 1;  /**< [31] Lock */
    } bit;
  } CR;
  union {
    __IO uint32_t reg;   /**< 0x14  Option control          */
    struct {
      __IO uint32_t OPTLOCK  : 1;  /**< [0] Option lock */
      __IO uint32_t OPTSTRT  : 1;  /**< [1] Option start */
      __IO uint32_t BOR_LEV  : 2;  /**< [3:2] BOR level */
      uint32_t               : 1;  /**< [4] Reserved */
      __IO uint32_t WDG_SW   : 1;  /**< [5] User option bytes */
      __IO uint32_t nRST_STOP: 1;  /**< [6] User option bytes */
      __IO uint32_t nRST_STDBY:1;  /**< [7] User option bytes */
      __IO uint32_t RDP      : 8;  /**< [15:8] Read protect */
      __IO uint32_t nWRP     : 12; /**< [27:16] Write protect */
      uint32_t               : 4;  /**< [31:28] Reserved */
    } bit;
  } OPTCR;
} FLASH_TypeDef;

/* ── PWR (Power Controller) ────────────────────────────────────────── */
typedef struct {
  union {
    __IO uint32_t reg;  /**< 0x00  Power control           */
    struct {
      __IO uint32_t LPDS     : 1;  /**< [0] Low-power deep sleep */
      __IO uint32_t PDDS     : 1;  /**< [1] Power down deep sleep */
      __IO uint32_t CWUF     : 1;  /**< [2] Clear wakeup flag */
      __IO uint32_t CSBF     : 1;  /**< [3] Clear standby flag */
      __IO uint32_t PVDE     : 1;  /**< [4] Power voltage detector enable */
      __IO uint32_t PLS      : 3;  /**< [7:5] PVD level selection */
      __IO uint32_t DBP      : 1;  /**< [8] Disable backup domain write protection */
      __IO uint32_t FPDS     : 1;  /**< [9] Flash power down in Stop mode */
      uint32_t               : 4;  /**< [13:10] Reserved */
      __IO uint32_t VOS      : 2;  /**< [15:14] Regulator voltage scaling output selection */
      uint32_t               : 16; /**< [31:16] Reserved */
    } bit;
  } CR;
  union {
    __IO uint32_t reg; /**< 0x04  Power control/status    */
    struct {
      __I  uint32_t WUF      : 1;  /**< [0] Wakeup flag */
      __I  uint32_t SBF      : 1;  /**< [1] Standby flag */
      __I  uint32_t PVDO     : 1;  /**< [2] PVD output */
      __I  uint32_t BRR      : 1;  /**< [3] Backup regulator ready */
      uint32_t               : 4;  /**< [7:4] Reserved */
      __IO uint32_t EWUP     : 1;  /**< [8] Enable WKUP pin */
      __IO uint32_t BRE      : 1;  /**< [9] Backup regulator enable */
      uint32_t               : 4;  /**< [13:10] Reserved */
      __I  uint32_t VOSRDY   : 1;  /**< [14] Regulator voltage scaling output ready */
      uint32_t               : 17; /**< [31:15] Reserved */
    } bit;
  } CSR;
} PWR_TypeDef;

/* ── GPIO (General Purpose I/O) ────────────────────────────────────── */
typedef struct {
  __IO uint32_t MODER;   /**< 0x00 GPIO port mode register                 */
  __IO uint32_t OTYPER;  /**< 0x04 GPIO port output type register            */
  __IO uint32_t OSPEEDR; /**< 0x08 GPIO port output speed register           */
  __IO uint32_t PUPDR;   /**< 0x0C GPIO port pull-up/pull-down register     */
  __I  uint32_t IDR;     /**< 0x10 GPIO port input data register            */
  __IO uint32_t ODR;     /**< 0x14 GPIO port output data register           */
  __O  uint32_t BSRR;    /**< 0x18 GPIO port bit set/reset register          */
  __IO uint32_t LCKR;    /**< 0x1C GPIO port configuration lock register     */
  __IO uint32_t AFR[2U]; /**< 0x20-0x24 GPIO alternate function registers    */
} GPIO_TypeDef;

/* ── I2C (Inter-integrated Circuit) ────────────────────────────────── */
/* Reference: RM0383 Rev 3, Chapter 18 */
typedef struct {
  __IO uint32_t CR1;   /**< 0x00 Control register 1 */
  __IO uint32_t CR2;   /**< 0x04 Control register 2 */
  __IO uint32_t OAR1;  /**< 0x08 Own address register 1 */
  __IO uint32_t OAR2;  /**< 0x0C Own address register 2 */
  __IO uint32_t DR;    /**< 0x10 Data register */
  __IO uint32_t SR1;   /**< 0x14 Status register 1 */
  __IO uint32_t SR2;   /**< 0x18 Status register 2 */
  __IO uint32_t CCR;   /**< 0x1C Clock control register */
  __IO uint32_t TRISE; /**< 0x20 TRISE register */
  __IO uint32_t FLTR;  /**< 0x24 FLTR register */
} I2C_TypeDef;

/* SPI/I2S (Serial Peripheral Interface). */
/* Reference: RM0383, Chapter 20. */
typedef struct {
  __IO uint32_t CR1;     /**< 0x00 SPI control register 1 */
  __IO uint32_t CR2;     /**< 0x04 SPI control register 2 */
  __I  uint32_t SR;      /**< 0x08 SPI status register */
  __IO uint32_t DR;      /**< 0x0C SPI data register */
  __IO uint32_t CRCPR;   /**< 0x10 CRC polynomial register */
  __I  uint32_t RXCRCR;  /**< 0x14 RX CRC register */
  __I  uint32_t TXCRCR;  /**< 0x18 TX CRC register */
  __IO uint32_t I2SCFGR; /**< 0x1C I2S configuration register */
  __IO uint32_t I2SPR;   /**< 0x20 I2S prescaler register */
} SPI_TypeDef;

/* ── USART (Universal Synchronous/Asynchronous Receiver Transmitter) ── */
/* Reference: RM0383 Rev 3, Chapter 19 */
typedef struct {
  union {
    __IO uint32_t reg;         /**< 0x00 Status register */
    struct {
      __I  uint32_t PE       : 1;  /**< [0] Parity error                    */
      __I  uint32_t FE       : 1;  /**< [1] Framing error                   */
      __I  uint32_t NF       : 1;  /**< [2] Noise detected flag             */
      __I  uint32_t ORE      : 1;  /**< [3] Overrun error                   */
      __I  uint32_t IDLE     : 1;  /**< [4] IDLE line detected              */
      __I  uint32_t RXNE     : 1;  /**< [5] Read data register not empty    */
      __IO uint32_t TC       : 1;  /**< [6] Transmission complete           */
      __I  uint32_t TXE      : 1;  /**< [7] Transmit data register empty    */
      __IO uint32_t LBD      : 1;  /**< [8] LIN break detection flag        */
      __IO uint32_t CTS      : 1;  /**< [9] CTS flag                        */
      uint32_t               : 22; /**< [31:10] Reserved                    */
    } bit;
  } SR;

  union {
    __IO uint32_t reg;         /**< 0x04 Data register */
    struct {
      __IO uint32_t DR       : 9;  /**< [8:0] Data value                    */
      uint32_t               : 23; /**< [31:9] Reserved                     */
    } bit;
  } DR;

  union {
    __IO uint32_t reg;         /**< 0x08 Baud rate register */
    struct {
      __IO uint32_t DIV_Fraction : 4;  /**< [3:0] Fraction of USARTDIV      */
      __IO uint32_t DIV_Mantissa : 12; /**< [15:4] Mantissa of USARTDIV     */
      uint32_t                   : 16; /**< [31:16] Reserved                */
    } bit;
  } BRR;

  union {
    __IO uint32_t reg;         /**< 0x0C Control register 1 */
    struct {
      __IO uint32_t SBK      : 1;  /**< [0] Send break                      */
      __IO uint32_t RWU      : 1;  /**< [1] Receiver wakeup                 */
      __IO uint32_t RE       : 1;  /**< [2] Receiver enable                 */
      __IO uint32_t TE       : 1;  /**< [3] Transmitter enable               */
      __IO uint32_t IDLEIE   : 1;  /**< [4] IDLE interrupt enable            */
      __IO uint32_t RXNEIE   : 1;  /**< [5] RXNE interrupt enable            */
      __IO uint32_t TCIE     : 1;  /**< [6] Transmission complete IE         */
      __IO uint32_t TXEIE    : 1;  /**< [7] TXE interrupt enable             */
      __IO uint32_t PEIE     : 1;  /**< [8] PE interrupt enable              */
      __IO uint32_t PS       : 1;  /**< [9] Parity selection (0=Even,1=Odd)  */
      __IO uint32_t PCE      : 1;  /**< [10] Parity control enable           */
      __IO uint32_t WAKE     : 1;  /**< [11] Wakeup method                  */
      __IO uint32_t M        : 1;  /**< [12] Word length (0=8bit, 1=9bit)   */
      __IO uint32_t UE       : 1;  /**< [13] USART enable                   */
      uint32_t               : 1;  /**< [14] Reserved                       */
      __IO uint32_t OVER8    : 1;  /**< [15] Oversampling mode (0=16x,1=8x) */
      uint32_t               : 16; /**< [31:16] Reserved                    */
    } bit;
  } CR1;

  union {
    __IO uint32_t reg;         /**< 0x10 Control register 2 */
    struct {
      __IO uint32_t ADD      : 4;  /**< [3:0] Address of the USART node     */
      uint32_t               : 1;  /**< [4] Reserved                        */
      __IO uint32_t LBDL     : 1;  /**< [5] LIN break detection length      */
      __IO uint32_t LBDIE    : 1;  /**< [6] LIN break detection IE          */
      uint32_t               : 1;  /**< [7] Reserved                        */
      __IO uint32_t LBCL     : 1;  /**< [8] Last bit clock pulse            */
      __IO uint32_t CPHA     : 1;  /**< [9] Clock phase                     */
      __IO uint32_t CPOL     : 1;  /**< [10] Clock polarity                 */
      __IO uint32_t CLKEN    : 1;  /**< [11] Clock enable                   */
      __IO uint32_t STOP     : 2;  /**< [13:12] STOP bits                   */
      __IO uint32_t LINEN    : 1;  /**< [14] LIN mode enable                */
      uint32_t               : 17; /**< [31:15] Reserved                    */
    } bit;
  } CR2;

  union {
    __IO uint32_t reg;         /**< 0x14 Control register 3 */
    struct {
      __IO uint32_t EIE      : 1;  /**< [0] Error interrupt enable           */
      __IO uint32_t IREN     : 1;  /**< [1] IrDA mode enable                */
      __IO uint32_t IRLP     : 1;  /**< [2] IrDA low-power                  */
      __IO uint32_t HDSEL    : 1;  /**< [3] Half-duplex selection            */
      __IO uint32_t NACK     : 1;  /**< [4] Smartcard NACK enable            */
      __IO uint32_t SCEN     : 1;  /**< [5] Smartcard mode enable            */
      __IO uint32_t DMAR     : 1;  /**< [6] DMA enable receiver              */
      __IO uint32_t DMAT     : 1;  /**< [7] DMA enable transmitter           */
      __IO uint32_t RTSE     : 1;  /**< [8] RTS enable                      */
      __IO uint32_t CTSE     : 1;  /**< [9] CTS enable                      */
      __IO uint32_t CTSIE    : 1;  /**< [10] CTS interrupt enable            */
      __IO uint32_t ONEBIT   : 1;  /**< [11] One sample bit method enable    */
      uint32_t               : 20; /**< [31:12] Reserved                    */
    } bit;
  } CR3;

  union {
    __IO uint32_t reg;         /**< 0x18 Guard time and prescaler register */
    struct {
      __IO uint32_t PSC      : 8;  /**< [7:0] Prescaler value               */
      __IO uint32_t GT       : 8;  /**< [15:8] Guard time value             */
      uint32_t               : 16; /**< [31:16] Reserved                    */
    } bit;
  } GTPR;
} USART_TypeDef;

/* ══════════════════════════════════════════════════════════════════════
 * Peripheral Instance Macros
 * ═════════════════════════════════════════════════════════════════════ */
#define RCC    ((RCC_TypeDef *)RCC_BASE)
#define FLASH  ((FLASH_TypeDef *)FLASH_R_BASE)
#define PWR    ((PWR_TypeDef *)PWR_BASE)

#define GPIOA  ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB  ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC  ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD  ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE  ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOH  ((GPIO_TypeDef *)GPIOH_BASE)

#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART2 ((USART_TypeDef *)USART2_BASE)
#define USART6 ((USART_TypeDef *)USART6_BASE)

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)
#define I2C2 ((I2C_TypeDef *)I2C2_BASE)
#define I2C3 ((I2C_TypeDef *)I2C3_BASE)

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI2 ((SPI_TypeDef *)SPI2_BASE)
#define SPI3 ((SPI_TypeDef *)SPI3_BASE)
#define SPI4 ((SPI_TypeDef *)SPI4_BASE)
#define SPI5 ((SPI_TypeDef *)SPI5_BASE)

/* ══════════════════════════════════════════════════════════════════════
 * RCC Bit Definitions
 * ═════════════════════════════════════════════════════════════════════ */

/* ── RCC_CR ────────────────────────────────────────────────────────── */
#define RCC_CR_HSION_Pos (0U)
#define RCC_CR_HSION (1U << RCC_CR_HSION_Pos)
#define RCC_CR_HSIRDY_Pos (1U)
#define RCC_CR_HSIRDY (1U << RCC_CR_HSIRDY_Pos)
#define RCC_CR_HSEON_Pos (16U)
#define RCC_CR_HSEON (1U << RCC_CR_HSEON_Pos)
#define RCC_CR_HSERDY_Pos (17U)
#define RCC_CR_HSERDY (1U << RCC_CR_HSERDY_Pos)
#define RCC_CR_HSEBYP_Pos (18U)
#define RCC_CR_HSEBYP (1U << RCC_CR_HSEBYP_Pos)
#define RCC_CR_CSSON_Pos (19U)
#define RCC_CR_CSSON (1U << RCC_CR_CSSON_Pos)
#define RCC_CR_PLLON_Pos (24U)
#define RCC_CR_PLLON (1U << RCC_CR_PLLON_Pos)
#define RCC_CR_PLLRDY_Pos (25U)
#define RCC_CR_PLLRDY (1U << RCC_CR_PLLRDY_Pos)
#define RCC_CR_PLLI2SON_Pos (26U)
#define RCC_CR_PLLI2SON (1U << RCC_CR_PLLI2SON_Pos)
#define RCC_CR_PLLI2SRDY_Pos (27U)
#define RCC_CR_PLLI2SRDY (1U << RCC_CR_PLLI2SRDY_Pos)

/* ── RCC_PLLCFGR ──────────────────────────────────────────────────── */
#define RCC_PLLCFGR_PLLM_Pos (0U)
#define RCC_PLLCFGR_PLLM_Msk (0x3FU << RCC_PLLCFGR_PLLM_Pos) /* [5:0]   */
#define RCC_PLLCFGR_PLLN_Pos (6U)
#define RCC_PLLCFGR_PLLN_Msk (0x1FFU << RCC_PLLCFGR_PLLN_Pos) /* [14:6]  */
#define RCC_PLLCFGR_PLLP_Pos (16U)
#define RCC_PLLCFGR_PLLP_Msk (0x3U << RCC_PLLCFGR_PLLP_Pos) /* [17:16] */
#define RCC_PLLCFGR_PLLSRC_Pos (22U)
#define RCC_PLLCFGR_PLLSRC (1U << RCC_PLLCFGR_PLLSRC_Pos)
#define RCC_PLLCFGR_PLLSRC_HSI (0U)
#define RCC_PLLCFGR_PLLSRC_HSE (1U << RCC_PLLCFGR_PLLSRC_Pos)
#define RCC_PLLCFGR_PLLQ_Pos (24U)
#define RCC_PLLCFGR_PLLQ_Msk (0xFU << RCC_PLLCFGR_PLLQ_Pos) /* [27:24] */

/* ── RCC_CFGR ─────────────────────────────────────────────────────── */
#define RCC_CFGR_SW_Pos (0U)
#define RCC_CFGR_SW_Msk (0x3U << RCC_CFGR_SW_Pos) /* [1:0]   */
#define RCC_CFGR_SW_HSI (0x0U << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_HSE (0x1U << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_PLL (0x2U << RCC_CFGR_SW_Pos)

#define RCC_CFGR_SWS_Pos (2U)
#define RCC_CFGR_SWS_Msk (0x3U << RCC_CFGR_SWS_Pos) /* [3:2]   */
#define RCC_CFGR_SWS_HSI (0x0U << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_HSE (0x1U << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_PLL (0x2U << RCC_CFGR_SWS_Pos)

#define RCC_CFGR_HPRE_Pos (4U)
#define RCC_CFGR_HPRE_Msk (0xFU << RCC_CFGR_HPRE_Pos) /* [7:4]   */

#define RCC_CFGR_PPRE1_Pos (10U)
#define RCC_CFGR_PPRE1_Msk (0x7U << RCC_CFGR_PPRE1_Pos) /* [12:10] */

#define RCC_CFGR_PPRE2_Pos (13U)
#define RCC_CFGR_PPRE2_Msk (0x7U << RCC_CFGR_PPRE2_Pos) /* [15:13] */

/* ── RCC_AHB1ENR ──────────────────────────────────────────────────── */
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)
#define RCC_AHB1ENR_GPIODEN (1U << 3)
#define RCC_AHB1ENR_GPIOEEN (1U << 4)
#define RCC_AHB1ENR_GPIOHEN (1U << 7)
#define RCC_AHB1ENR_CRCEN (1U << 12)
#define RCC_AHB1ENR_DMA1EN (1U << 21)
#define RCC_AHB1ENR_DMA2EN (1U << 22)

/* ── RCC_AHB2ENR ──────────────────────────────────────────────────── */
#define RCC_AHB2ENR_OTGFSEN (1U << 7)

/* ── RCC_APB1ENR ──────────────────────────────────────────────────── */
#define RCC_APB1ENR_TIM2EN (1U << 0)
#define RCC_APB1ENR_TIM3EN (1U << 1)
#define RCC_APB1ENR_TIM4EN (1U << 2)
#define RCC_APB1ENR_TIM5EN (1U << 3)
#define RCC_APB1ENR_WWDGEN (1U << 11)
#define RCC_APB1ENR_SPI2EN (1U << 14)
#define RCC_APB1ENR_SPI3EN (1U << 15)
#define RCC_APB1ENR_USART2EN (1U << 17)
#define RCC_APB1ENR_I2C1EN (1U << 21)
#define RCC_APB1ENR_I2C2EN (1U << 22)
#define RCC_APB1ENR_I2C3EN (1U << 23)
#define RCC_APB1ENR_PWREN (1U << 28)

/* ── RCC_APB2ENR ──────────────────────────────────────────────────── */
#define RCC_APB2ENR_TIM1EN (1U << 0)
#define RCC_APB2ENR_USART1EN (1U << 4)
#define RCC_APB2ENR_USART6EN (1U << 5)
#define RCC_APB2ENR_ADC1EN (1U << 8)
#define RCC_APB2ENR_SDIOEN (1U << 11)
#define RCC_APB2ENR_SPI1EN (1U << 12)
#define RCC_APB2ENR_SPI4EN (1U << 13)
#define RCC_APB2ENR_SYSCFGEN (1U << 14)
#define RCC_APB2ENR_TIM9EN (1U << 16)
#define RCC_APB2ENR_TIM10EN (1U << 17)
#define RCC_APB2ENR_TIM11EN (1U << 18)
#define RCC_APB2ENR_SPI5EN (1U << 20)

/* ── RCC_AHB1RSTR ─────────────────────────────────────────────────── */
#define RCC_AHB1RSTR_GPIOARST (1U << 0)
#define RCC_AHB1RSTR_GPIOBRST (1U << 1)
#define RCC_AHB1RSTR_GPIOCRST (1U << 2)
#define RCC_AHB1RSTR_GPIODRST (1U << 3)
#define RCC_AHB1RSTR_GPIOERST (1U << 4)
#define RCC_AHB1RSTR_GPIOHRST (1U << 7)
#define RCC_AHB1RSTR_CRCRST (1U << 12)
#define RCC_AHB1RSTR_DMA1RST (1U << 21)
#define RCC_AHB1RSTR_DMA2RST (1U << 22)

/* ── RCC_APB1RSTR ─────────────────────────────────────────────────── */
#define RCC_APB1RSTR_TIM2RST (1U << 0)
#define RCC_APB1RSTR_TIM3RST (1U << 1)
#define RCC_APB1RSTR_TIM4RST (1U << 2)
#define RCC_APB1RSTR_TIM5RST (1U << 3)
#define RCC_APB1RSTR_WWDGRST (1U << 11)
#define RCC_APB1RSTR_SPI2RST (1U << 14)
#define RCC_APB1RSTR_SPI3RST (1U << 15)
#define RCC_APB1RSTR_USART2RST (1U << 17)
#define RCC_APB1RSTR_I2C1RST (1U << 21)
#define RCC_APB1RSTR_I2C2RST (1U << 22)
#define RCC_APB1RSTR_I2C3RST (1U << 23)

/* ── I2C register bit definitions ──────────────────────────────────── */
#define I2C_CR1_PE        (1U << 0)
#define I2C_CR1_START     (1U << 8)
#define I2C_CR1_STOP      (1U << 9)
#define I2C_CR1_ACK       (1U << 10)
#define I2C_CR1_POS       (1U << 11)
#define I2C_CR1_SWRST     (1U << 15)

#define I2C_CR2_FREQ_Msk  (0x3FU << 0)

#define I2C_OAR1_ADD0     (1U << 0)
#define I2C_OAR1_ADD7_Msk (0x7FU << 1)
#define I2C_OAR1_ADDMODE  (1U << 15)
#define I2C_OAR1_BIT14    (1U << 14)

#define I2C_SR1_SB        (1U << 0)
#define I2C_SR1_ADDR      (1U << 1)
#define I2C_SR1_BTF       (1U << 2)
#define I2C_SR1_ADD10     (1U << 3)
#define I2C_SR1_STOPF     (1U << 4)
#define I2C_SR1_RXNE      (1U << 6)
#define I2C_SR1_TXE       (1U << 7)
#define I2C_SR1_BERR      (1U << 8)
#define I2C_SR1_ARLO      (1U << 9)
#define I2C_SR1_AF        (1U << 10)
#define I2C_SR1_OVR       (1U << 11)
#define I2C_SR1_TIMEOUT   (1U << 14)

#define I2C_SR2_MSL       (1U << 0)
#define I2C_SR2_BUSY      (1U << 1)
#define I2C_SR2_TRA       (1U << 2)

#define I2C_CCR_CCR_Msk   (0xFFFU << 0)
#define I2C_CCR_DUTY      (1U << 14)
#define I2C_CCR_FS        (1U << 15)

#define I2C_TRISE_TRISE_Msk (0x3FU << 0)
#define RCC_APB1RSTR_PWRRST (1U << 28)

/* ── RCC_APB2RSTR ─────────────────────────────────────────────────── */
#define RCC_APB2RSTR_TIM1RST (1U << 0)
#define RCC_APB2RSTR_USART1RST (1U << 4)
#define RCC_APB2RSTR_USART6RST (1U << 5)
#define RCC_APB2RSTR_ADC1RST (1U << 8)
#define RCC_APB2RSTR_SDIORST (1U << 11)
#define RCC_APB2RSTR_SPI1RST (1U << 12)
#define RCC_APB2RSTR_SPI4RST (1U << 13)
#define RCC_APB2RSTR_SYSCFGRST (1U << 14)
#define RCC_APB2RSTR_TIM9RST (1U << 16)
#define RCC_APB2RSTR_TIM10RST (1U << 17)
#define RCC_APB2RSTR_TIM11RST (1U << 18)
#define RCC_APB2RSTR_SPI5RST (1U << 20)

/* ══════════════════════════════════════════════════════════════════════
 * FLASH Bit Definitions
 * ═════════════════════════════════════════════════════════════════════ */

/* ── FLASH_ACR ─────────────────────────────────────────────────────── */
#define FLASH_ACR_LATENCY_Pos (0U)
#define FLASH_ACR_LATENCY_Msk (0xFU << FLASH_ACR_LATENCY_Pos)
#define FLASH_ACR_PRFTEN (1U << 8) /**< Prefetch enable        */
#define FLASH_ACR_ICEN (1U << 9)   /**< Instruction cache en   */
#define FLASH_ACR_DCEN (1U << 10)  /**< Data cache enable      */
#define FLASH_ACR_ICRST (1U << 11) /**< Instruction cache rst  */
#define FLASH_ACR_DCRST (1U << 12) /**< Data cache reset       */

/* ══════════════════════════════════════════════════════════════════════
 * PWR Bit Definitions
 * ═════════════════════════════════════════════════════════════════════ */

/* ── PWR_CR ────────────────────────────────────────────────────────── */
#define PWR_CR_VOS_Pos (14U)
#define PWR_CR_VOS_Msk (0x3U << PWR_CR_VOS_Pos)
#define PWR_CR_VOS_SCALE1 (0x3U << PWR_CR_VOS_Pos) /**< Max freq ≤100 MHz */
#define PWR_CR_VOS_SCALE2 (0x2U << PWR_CR_VOS_Pos) /**< Max freq ≤84 MHz  */
#define PWR_CR_VOS_SCALE3 (0x1U << PWR_CR_VOS_Pos) /**< Max freq ≤64 MHz  */

/* ══════════════════════════════════════════════════════════════════════
 * GPIO Bit Definitions
 * ═════════════════════════════════════════════════════════════════════ */

/* ── GPIO_LCKR ─────────────────────────────────────────────────────── */
#define GPIO_LCKR_LCKK_Pos (16U)
#define GPIO_LCKR_LCKK     (1U << GPIO_LCKR_LCKK_Pos)

/* ══════════════════════════════════════════════════════════════════════
 * USART Bit Definitions
 * Reference: RM0383 Rev 3, §19.6
 * ═════════════════════════════════════════════════════════════════════ */

/* ── USART_SR ──────────────────────────────────────────────────────── */
#define USART_SR_PE_Pos   (0U)
#define USART_SR_PE       (1U << USART_SR_PE_Pos)   /**< Parity error        */
#define USART_SR_FE_Pos   (1U)
#define USART_SR_FE       (1U << USART_SR_FE_Pos)   /**< Framing error       */
#define USART_SR_NF_Pos   (2U)
#define USART_SR_NF       (1U << USART_SR_NF_Pos)   /**< Noise detected      */
#define USART_SR_ORE_Pos  (3U)
#define USART_SR_ORE      (1U << USART_SR_ORE_Pos)  /**< Overrun error       */
#define USART_SR_IDLE_Pos (4U)
#define USART_SR_IDLE     (1U << USART_SR_IDLE_Pos)  /**< IDLE line detected  */
#define USART_SR_RXNE_Pos (5U)
#define USART_SR_RXNE     (1U << USART_SR_RXNE_Pos)  /**< RX not empty        */
#define USART_SR_TC_Pos   (6U)
#define USART_SR_TC       (1U << USART_SR_TC_Pos)   /**< Transmission complete*/
#define USART_SR_TXE_Pos  (7U)
#define USART_SR_TXE      (1U << USART_SR_TXE_Pos)  /**< TX data reg empty   */
#define USART_SR_LBD_Pos  (8U)
#define USART_SR_LBD      (1U << USART_SR_LBD_Pos)  /**< LIN break detected  */
#define USART_SR_CTS_Pos  (9U)
#define USART_SR_CTS      (1U << USART_SR_CTS_Pos)  /**< CTS flag            */

/* ── USART_BRR ─────────────────────────────────────────────────────── */
#define USART_BRR_DIV_Fraction_Pos (0U)
#define USART_BRR_DIV_Fraction_Msk (0xFU << USART_BRR_DIV_Fraction_Pos)
#define USART_BRR_DIV_Mantissa_Pos (4U)
#define USART_BRR_DIV_Mantissa_Msk (0xFFFU << USART_BRR_DIV_Mantissa_Pos)

/* ── USART_CR1 ─────────────────────────────────────────────────────── */
#define USART_CR1_SBK_Pos    (0U)
#define USART_CR1_SBK        (1U << USART_CR1_SBK_Pos)
#define USART_CR1_RWU_Pos    (1U)
#define USART_CR1_RWU        (1U << USART_CR1_RWU_Pos)
#define USART_CR1_RE_Pos     (2U)
#define USART_CR1_RE         (1U << USART_CR1_RE_Pos)
#define USART_CR1_TE_Pos     (3U)
#define USART_CR1_TE         (1U << USART_CR1_TE_Pos)
#define USART_CR1_IDLEIE_Pos (4U)
#define USART_CR1_IDLEIE     (1U << USART_CR1_IDLEIE_Pos)
#define USART_CR1_RXNEIE_Pos (5U)
#define USART_CR1_RXNEIE     (1U << USART_CR1_RXNEIE_Pos)
#define USART_CR1_TCIE_Pos   (6U)
#define USART_CR1_TCIE       (1U << USART_CR1_TCIE_Pos)
#define USART_CR1_TXEIE_Pos  (7U)
#define USART_CR1_TXEIE      (1U << USART_CR1_TXEIE_Pos)
#define USART_CR1_PEIE_Pos   (8U)
#define USART_CR1_PEIE       (1U << USART_CR1_PEIE_Pos)
#define USART_CR1_PS_Pos     (9U)
#define USART_CR1_PS         (1U << USART_CR1_PS_Pos)
#define USART_CR1_PCE_Pos    (10U)
#define USART_CR1_PCE        (1U << USART_CR1_PCE_Pos)
#define USART_CR1_WAKE_Pos   (11U)
#define USART_CR1_WAKE       (1U << USART_CR1_WAKE_Pos)
#define USART_CR1_M_Pos      (12U)
#define USART_CR1_M          (1U << USART_CR1_M_Pos)
#define USART_CR1_UE_Pos     (13U)
#define USART_CR1_UE         (1U << USART_CR1_UE_Pos)
#define USART_CR1_OVER8_Pos  (15U)
#define USART_CR1_OVER8      (1U << USART_CR1_OVER8_Pos)

/* ── USART_CR2 ─────────────────────────────────────────────────────── */
#define USART_CR2_ADD_Pos    (0U)
#define USART_CR2_ADD_Msk    (0xFU << USART_CR2_ADD_Pos)
#define USART_CR2_LBDL_Pos   (5U)
#define USART_CR2_LBDL       (1U << USART_CR2_LBDL_Pos)
#define USART_CR2_LBDIE_Pos  (6U)
#define USART_CR2_LBDIE      (1U << USART_CR2_LBDIE_Pos)
#define USART_CR2_LBCL_Pos   (8U)
#define USART_CR2_LBCL       (1U << USART_CR2_LBCL_Pos)
#define USART_CR2_CPHA_Pos   (9U)
#define USART_CR2_CPHA       (1U << USART_CR2_CPHA_Pos)
#define USART_CR2_CPOL_Pos   (10U)
#define USART_CR2_CPOL       (1U << USART_CR2_CPOL_Pos)
#define USART_CR2_CLKEN_Pos  (11U)
#define USART_CR2_CLKEN      (1U << USART_CR2_CLKEN_Pos)
#define USART_CR2_STOP_Pos   (12U)
#define USART_CR2_STOP_Msk   (0x3U << USART_CR2_STOP_Pos)
#define USART_CR2_LINEN_Pos  (14U)
#define USART_CR2_LINEN      (1U << USART_CR2_LINEN_Pos)

/* ── USART_CR3 ─────────────────────────────────────────────────────── */
#define USART_CR3_EIE_Pos    (0U)
#define USART_CR3_EIE        (1U << USART_CR3_EIE_Pos)
#define USART_CR3_IREN_Pos   (1U)
#define USART_CR3_IREN       (1U << USART_CR3_IREN_Pos)
#define USART_CR3_IRLP_Pos   (2U)
#define USART_CR3_IRLP       (1U << USART_CR3_IRLP_Pos)
#define USART_CR3_HDSEL_Pos  (3U)
#define USART_CR3_HDSEL      (1U << USART_CR3_HDSEL_Pos)
#define USART_CR3_NACK_Pos   (4U)
#define USART_CR3_NACK       (1U << USART_CR3_NACK_Pos)
#define USART_CR3_SCEN_Pos   (5U)
#define USART_CR3_SCEN       (1U << USART_CR3_SCEN_Pos)
#define USART_CR3_DMAR_Pos   (6U)
#define USART_CR3_DMAR       (1U << USART_CR3_DMAR_Pos)
#define USART_CR3_DMAT_Pos   (7U)
#define USART_CR3_DMAT       (1U << USART_CR3_DMAT_Pos)
#define USART_CR3_RTSE_Pos   (8U)
#define USART_CR3_RTSE       (1U << USART_CR3_RTSE_Pos)
#define USART_CR3_CTSE_Pos   (9U)
#define USART_CR3_CTSE       (1U << USART_CR3_CTSE_Pos)
#define USART_CR3_CTSIE_Pos  (10U)
#define USART_CR3_CTSIE      (1U << USART_CR3_CTSIE_Pos)
#define USART_CR3_ONEBIT_Pos (11U)
#define USART_CR3_ONEBIT     (1U << USART_CR3_ONEBIT_Pos)

/* ══════════════════════════════════════════════════════════════════════
 * Cortex-M4 Core — SysTick Timer
 * Reference: ARMv7-M Architecture Reference Manual §B3.3
 * ═════════════════════════════════════════════════════════════════════ */

/* ── Memory Map ────────────────────────────────────────────────────── */
#define SCS_BASE 0xE000E000UL
#define SYSTICK_BASE (SCS_BASE + 0x0010UL)

/* ── Register Structure ────────────────────────────────────────────── */
typedef struct {
  union {
    __IO uint32_t reg;    /**< 0x00  Control and Status register (whole register) */
    struct {
      __IO uint32_t ENABLE    : 1;  /**< [0] Counter enable              */
      __IO uint32_t TICKINT   : 1;  /**< [1] Interrupt on count to 0     */
      __IO uint32_t CLKSOURCE : 1;  /**< [2] 1=AHB clock, 0=AHB/8        */
      uint32_t                : 13; /**< [15:3] Reserved                 */
      __I  uint32_t COUNTFLAG : 1;  /**< [16] 1 if counted to 0          */
      uint32_t                : 15; /**< [31:17] Reserved                */
    } bit;
  } CTRL;
  union {
    __IO uint32_t reg;    /**< 0x04  Reload Value register (whole register) */
    struct {
      __IO uint32_t RELOAD    : 24; /**< [23:0] 24-bit reload value      */
      uint32_t                : 8;  /**< [31:24] Reserved                */
    } bit;
  } LOAD;
  union {
    __IO uint32_t reg;    /**< 0x08  Current Value register (whole register) */
    struct {
      __IO uint32_t CURRENT   : 24; /**< [23:0] 24-bit current value     */
      uint32_t                : 8;  /**< [31:24] Reserved                */
    } bit;
  } VAL;
  union {
    __I  uint32_t reg;    /**< 0x0C  Calibration Value register (whole register) */
    struct {
      __I  uint32_t TENMS     : 24; /**< [23:0] 10ms calibration value   */
      uint32_t                : 6;  /**< [29:24] Reserved                */
      __I  uint32_t SKEW      : 1;  /**< [30] Calibration value is skewed*/
      __I  uint32_t NOREF     : 1;  /**< [31] No reference clock         */
    } bit;
  } CALIB;
} SysTick_TypeDef;

/* ── Instance Macro ────────────────────────────────────────────────── */
#define SYSTICK ((SysTick_TypeDef *)SYSTICK_BASE)

/* ── SysTick_CTRL bit definitions ──────────────────────────────────── */
#define SYSTICK_CTRL_ENABLE_Pos (0U)
#define SYSTICK_CTRL_ENABLE                                                    \
  (1U << SYSTICK_CTRL_ENABLE_Pos) /**< Counter enable              */
#define SYSTICK_CTRL_TICKINT_Pos (1U)
#define SYSTICK_CTRL_TICKINT                                                   \
  (1U << SYSTICK_CTRL_TICKINT_Pos) /**< Interrupt on count to 0     */
#define SYSTICK_CTRL_CLKSOURCE_Pos (2U)
#define SYSTICK_CTRL_CLKSOURCE                                                 \
  (1U << SYSTICK_CTRL_CLKSOURCE_Pos) /**< 1=AHB, 0=AHB/8             */
#define SYSTICK_CTRL_COUNTFLAG_Pos (16U)
#define SYSTICK_CTRL_COUNTFLAG                                                 \
  (1U << SYSTICK_CTRL_COUNTFLAG_Pos) /**< 1 if counted to 0 since read*/

/* ── SysTick_LOAD bit definitions ──────────────────────────────────── */
#define SYSTICK_LOAD_RELOAD_Pos (0U)
#define SYSTICK_LOAD_RELOAD_Msk                                                \
  (0x00FFFFFFU << SYSTICK_LOAD_RELOAD_Pos) /**< 24-bit reload value */

/* ── SysTick_CALIB bit definitions ─────────────────────────────────── */
#define SYSTICK_CALIB_TENMS_Msk 0x00FFFFFFU /**< 10ms calibration value     */
#define SYSTICK_CALIB_SKEW_Pos (30U)
#define SYSTICK_CALIB_SKEW                                                     \
  (1U << SYSTICK_CALIB_SKEW_Pos) /**< TENMS not exactly 10ms     */
#define SYSTICK_CALIB_NOREF_Pos (31U)
#define SYSTICK_CALIB_NOREF                                                    \
  (1U << SYSTICK_CALIB_NOREF_Pos) /**< No external ref clock      */

/* ══════════════════════════════════════════════════════════════════════
 * Cortex-M4 Core — NVIC (Nested Vectored Interrupt Controller)
 * Reference: ARMv7-M Architecture Reference Manual §B3.4
 * ═════════════════════════════════════════════════════════════════════ */

/* ── Memory Map ────────────────────────────────────────────────────── */
#define NVIC_BASE (SCS_BASE + 0x0100UL)

/* ── Register Structure ────────────────────────────────────────────── */
typedef struct {
  __IO uint32_t ISER[8U];               /**< Offset: 0x000 (R/W)  Interrupt Set Enable Register */
        uint32_t RESERVED0[24U];
  __IO uint32_t ICER[8U];               /**< Offset: 0x080 (R/W)  Interrupt Clear Enable Register */
        uint32_t RESERVED1[24U];
  __IO uint32_t ISPR[8U];               /**< Offset: 0x100 (R/W)  Interrupt Set Pending Register */
        uint32_t RESERVED2[24U];
  __IO uint32_t ICPR[8U];               /**< Offset: 0x180 (R/W)  Interrupt Clear Pending Register */
        uint32_t RESERVED3[24U];
  __IO uint32_t IABR[8U];               /**< Offset: 0x200 (R/W)  Interrupt Active Bit Register */
        uint32_t RESERVED4[56U];
  __IO uint8_t  IP[240U];               /**< Offset: 0x300 (R/W)  Interrupt Priority Register (8-bit wide) */
        uint32_t RESERVED5[644U];
  __O  uint32_t STIR;                   /**< Offset: 0xE00 ( /W)  Software Trigger Interrupt Register */
} NVIC_TypeDef;

/* ── Instance Macro ────────────────────────────────────────────────── */
#define NVIC ((NVIC_TypeDef *)NVIC_BASE)

/* ══════════════════════════════════════════════════════════════════════
 * Cortex-M4 Core — System Control Block (SCB)
 * Reference: ARMv7-M Architecture Reference Manual §B3.2
 * ═════════════════════════════════════════════════════════════════════ */

/* ── Memory Map ────────────────────────────────────────────────────── */
#define SCB_BASE (SCS_BASE + 0x0D00UL)

/* ── Register Structure ────────────────────────────────────────────── */
typedef struct {
  __I  uint32_t CPUID;                  /**< Offset: 0x000 (R/ )  CPUID Base Register */
  __IO uint32_t ICSR;                   /**< Offset: 0x004 (R/W)  Interrupt Control and State Register */
  __IO uint32_t VTOR;                   /**< Offset: 0x008 (R/W)  Vector Table Offset Register */
  __IO uint32_t AIRCR;                  /**< Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register */
  __IO uint32_t SCR;                    /**< Offset: 0x010 (R/W)  System Control Register */
  __IO uint32_t CCR;                    /**< Offset: 0x014 (R/W)  Configuration and Control Register */
  __IO uint8_t  SHP[12U];               /**< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
  __IO uint32_t SHCSR;                  /**< Offset: 0x024 (R/W)  System Handler Control and State Register */
  __IO uint32_t CFSR;                   /**< Offset: 0x028 (R/W)  Configurable Fault Status Register */
  __IO uint32_t HFSR;                   /**< Offset: 0x02C (R/W)  HardFault Status Register */
  __IO uint32_t DFSR;                   /**< Offset: 0x030 (R/W)  Debug Fault Status Register */
  __IO uint32_t MMFAR;                  /**< Offset: 0x034 (R/W)  MemManage Fault Address Register */
  __IO uint32_t BFAR;                   /**< Offset: 0x038 (R/W)  BusFault Address Register */
  __IO uint32_t AFSR;                   /**< Offset: 0x03C (R/W)  Auxiliary Fault Status Register */
        uint32_t RESERVED0[18U];        /**< Offset: 0x040 */
  __IO uint32_t CPACR;                  /**< Offset: 0x088 (R/W)  Coprocessor Access Control Register */
} SCB_TypeDef;

/* ── Instance Macro ────────────────────────────────────────────────── */
#define SCB ((SCB_TypeDef *)SCB_BASE)

/* ── SCB_AIRCR bit definitions ─────────────────────────────────────── */
#define SCB_AIRCR_VECTKEY_Pos       (16U)
#define SCB_AIRCR_VECTKEY_Msk       (0xFFFFU << SCB_AIRCR_VECTKEY_Pos)
#define SCB_AIRCR_PRIGROUP_Pos      (8U)
#define SCB_AIRCR_PRIGROUP_Msk      (7U << SCB_AIRCR_PRIGROUP_Pos)

/* SPI_CR1 bit definitions */
#define SPI_CR1_CPHA       (1U << 0U)
#define SPI_CR1_CPOL       (1U << 1U)
#define SPI_CR1_MSTR       (1U << 2U)
#define SPI_CR1_BR_Pos     (3U)
#define SPI_CR1_BR_Msk     (7U << SPI_CR1_BR_Pos)
#define SPI_CR1_SPE        (1U << 6U)
#define SPI_CR1_LSBFIRST   (1U << 7U)
#define SPI_CR1_SSI        (1U << 8U)
#define SPI_CR1_SSM        (1U << 9U)
#define SPI_CR1_DFF        (1U << 11U)
#define SPI_CR1_CRCEN      (1U << 13U)
#define SPI_CR1_BIDIOE     (1U << 14U)
#define SPI_CR1_BIDIMODE   (1U << 15U)

/* SPI_CR2 bit definitions */
#define SPI_CR2_RXDMAEN    (1U << 0U)
#define SPI_CR2_TXDMAEN    (1U << 1U)
#define SPI_CR2_SSOE       (1U << 2U)
#define SPI_CR2_ERRIE      (1U << 5U)
#define SPI_CR2_RXNEIE     (1U << 6U)
#define SPI_CR2_TXEIE      (1U << 7U)

/* SPI_SR bit definitions */
#define SPI_SR_RXNE        (1U << 0U)
#define SPI_SR_TXE         (1U << 1U)
#define SPI_SR_CRCERR       (1U << 4U)
#define SPI_SR_MODF         (1U << 5U)
#define SPI_SR_OVR          (1U << 6U)
#define SPI_SR_BSY          (1U << 7U)

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_XE_H */
