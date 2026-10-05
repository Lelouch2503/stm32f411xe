/**
 * @file    stm32f411_rcc.h
 * @brief   RCC driver for STM32F411xE — public API
 *
 * Provides functions to:
 *  - Configure the system clock tree (HSI / HSE / PLL)
 *  - Read back actual clock frequencies (SYSCLK, HCLK, PCLK1, PCLK2)
 *  - Enable / disable / reset peripheral clocks on AHB1, APB1, APB2
 *
 * All enum types carry the exact register field values, so the user
 * never has to deal with raw hex — just pick the meaningful name.
 */

#ifndef STM32F411_RCC_H
#define STM32F411_RCC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f411_xe.h"

/* ══════════════════════════════════════════════════════════════════════
 * Enumerations — descriptive values for user clarity
 * ═════════════════════════════════════════════════════════════════════ */

/** System clock source selection (maps to RCC_CFGR SW[1:0]) */
typedef enum {
  RCC_SYSCLK_HSI = 0U, /**< 16 MHz internal RC oscillator   */
  RCC_SYSCLK_HSE = 1U, /**< External crystal (HSE_VALUE)    */
  RCC_SYSCLK_PLL = 2U  /**< PLL output                      */
} RCC_SysClkSrc_t;

/** PLL input clock source (maps to PLLCFGR PLLSRC bit) */
typedef enum {
  RCC_PLLSRC_HSI = 0U, /**< HSI 16 MHz feeds PLL            */
  RCC_PLLSRC_HSE = 1U  /**< HSE crystal feeds PLL           */
} RCC_PLL_Source_t;

/** PLL P division factor (maps to PLLCFGR PLLP[1:0] encoding) */
typedef enum {
  RCC_PLLP_DIV2 = 0U, /**< PLLCLK = VCO / 2               */
  RCC_PLLP_DIV4 = 1U, /**< PLLCLK = VCO / 4               */
  RCC_PLLP_DIV6 = 2U, /**< PLLCLK = VCO / 6               */
  RCC_PLLP_DIV8 = 3U  /**< PLLCLK = VCO / 8               */
} RCC_PLLP_t;

/** AHB bus prescaler — HPRE field value (CFGR[7:4]) */
typedef enum {
  RCC_AHB_DIV1 = 0x0U,   /**< SYSCLK not divided              */
  RCC_AHB_DIV2 = 0x8U,   /**< SYSCLK / 2                      */
  RCC_AHB_DIV4 = 0x9U,   /**< SYSCLK / 4                      */
  RCC_AHB_DIV8 = 0xAU,   /**< SYSCLK / 8                      */
  RCC_AHB_DIV16 = 0xBU,  /**< SYSCLK / 16                     */
  RCC_AHB_DIV64 = 0xCU,  /**< SYSCLK / 64                     */
  RCC_AHB_DIV128 = 0xDU, /**< SYSCLK / 128                    */
  RCC_AHB_DIV256 = 0xEU, /**< SYSCLK / 256                    */
  RCC_AHB_DIV512 = 0xFU  /**< SYSCLK / 512                    */
} RCC_AHB_Prescaler_t;

/** APB bus prescaler — PPREx field value (CFGR[12:10] / CFGR[15:13]) */
typedef enum {
  RCC_APB_DIV1 = 0x0U, /**< HCLK not divided                */
  RCC_APB_DIV2 = 0x4U, /**< HCLK / 2                        */
  RCC_APB_DIV4 = 0x5U, /**< HCLK / 4                        */
  RCC_APB_DIV8 = 0x6U, /**< HCLK / 8                        */
  RCC_APB_DIV16 = 0x7U /**< HCLK / 16                       */
} RCC_APB_Prescaler_t;

/** Flash latency / wait states (at V_DD = 2.7–3.6 V) */
typedef enum {
  RCC_FLASH_LATENCY_0WS = 0U, /**< 0 WS — HCLK ≤  30 MHz          */
  RCC_FLASH_LATENCY_1WS = 1U, /**< 1 WS — HCLK ≤  64 MHz          */
  RCC_FLASH_LATENCY_2WS = 2U, /**< 2 WS — HCLK ≤  90 MHz          */
  RCC_FLASH_LATENCY_3WS = 3U  /**< 3 WS — HCLK ≤ 100 MHz          */
} RCC_Flash_Latency_t;

/* ══════════════════════════════════════════════════════════════════════
 * Configuration Structures
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief PLL configuration parameters.
 *
 * Formulas (RM0383 §6.3.2):
 *   VCO_input  = PLL_source / PLLM        (must be 1–2 MHz)
 *   VCO_output = VCO_input  × PLLN        (must be 100–432 MHz)
 *   PLLCLK     = VCO_output / PLLP        (system clock)
 *   PLL48CLK   = VCO_output / PLLQ        (USB / SDIO, ideally 48 MHz)
 */
typedef struct {
  RCC_PLL_Source_t PLL_Source; /**< PLL input: HSI or HSE             */
  uint32_t PLLM;               /**< VCO input divider       (2..63)   */
  uint32_t PLLN;               /**< VCO multiplier          (50..432) */
  RCC_PLLP_t PLLP;             /**< System clock divider    (2,4,6,8) */
  uint32_t PLLQ;               /**< USB / SDIO divider      (2..15)   */
} RCC_PLL_Config_t;

/**
 * @brief Complete system clock configuration.
 *
 * Fill in this struct and pass to rcc_sys_clk_config().
 * The driver will handle the correct sequencing of:
 *   oscillator start → PLL lock → flash latency → prescalers → clock switch.
 */
typedef struct {
  RCC_SysClkSrc_t sysclk_src;         /**< System clock source          */
  RCC_AHB_Prescaler_t ahb_prescaler;  /**< AHB  prescaler (HPRE)        */
  RCC_APB_Prescaler_t apb1_prescaler; /**< APB1 prescaler (PPRE1, max 50 MHz) */
  RCC_APB_Prescaler_t apb2_prescaler; /**< APB2 prescaler (PPRE2, max 100 MHz)*/
  RCC_Flash_Latency_t flash_latency;  /**< Flash wait states             */
  RCC_PLL_Config_t pll; /**< PLL parameters (used when src == PLL) */
} RCC_ClkInit_t;

/* ══════════════════════════════════════════════════════════════════════
 * Public API — System Clock Configuration
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Configure the full system clock tree.
 *
 * Sequence (for PLL mode):
 *   1. Enable PLL source oscillator (HSE/HSI), wait for ready
 *   2. Enable PWR clock, set voltage regulator to Scale 1
 *   3. Configure PLL parameters (PLLM, PLLN, PLLP, PLLQ, PLLSRC)
 *   4. Enable PLL, wait for lock
 *   5. Set flash latency + caches
 *   6. Set AHB / APB1 / APB2 prescalers
 *   7. Switch system clock, wait for confirmation
 *
 * @param  cfg  Pointer to a filled-in RCC_ClkInit_t.
 * @retval  0   Success
 * @retval <0   Timeout error code (see implementation for details)
 */
int rcc_sys_clk_config(const RCC_ClkInit_t *cfg);

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Clock Frequency Readback
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Return the current SYSCLK frequency in Hz.
 *         Decodes SWS bits and, if PLL, computes from PLLCFGR fields.
 */
uint32_t rcc_get_sysclk_freq(void);

/**
 * @brief  Return the current HCLK (AHB bus / core) frequency in Hz.
 */
uint32_t rcc_get_hclk_freq(void);

/**
 * @brief  Return the current APB1 peripheral clock (PCLK1) in Hz.
 *         Note: APB1 timer clocks are 2× PCLK1 if prescaler ≠ 1.
 */
uint32_t rcc_get_pclk1_freq(void);

/**
 * @brief  Return the current APB2 peripheral clock (PCLK2) in Hz.
 *         Note: APB2 timer clocks are 2× PCLK2 if prescaler ≠ 1.
 */
uint32_t rcc_get_pclk2_freq(void);

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Peripheral Clock Enable / Disable / Reset
 * ═════════════════════════════════════════════════════════════════════ */

/** @brief Enable  peripheral clock on AHB1 bus.  @param periph  Bit mask, e.g.
 * RCC_AHB1ENR_GPIODEN */
void rcc_ahb1_clk_enable(uint32_t periph);
/** @brief Disable peripheral clock on AHB1 bus. */
void rcc_ahb1_clk_disable(uint32_t periph);

/** @brief Enable  peripheral clock on APB1 bus.  @param periph  Bit mask, e.g.
 * RCC_APB1ENR_USART2EN */
void rcc_apb1_clk_enable(uint32_t periph);
/** @brief Disable peripheral clock on APB1 bus. */
void rcc_apb1_clk_disable(uint32_t periph);

/** @brief Enable  peripheral clock on APB2 bus.  @param periph  Bit mask, e.g.
 * RCC_APB2ENR_SPI1EN */
void rcc_apb2_clk_enable(uint32_t periph);
/** @brief Disable peripheral clock on APB2 bus. */
void rcc_apb2_clk_disable(uint32_t periph);

/** @brief Assert then de-assert reset for a peripheral on AHB1. */
void rcc_ahb1_periph_reset(uint32_t periph);
/** @brief Assert then de-assert reset for a peripheral on APB1. */
void rcc_apb1_periph_reset(uint32_t periph);
/** @brief Assert then de-assert reset for a peripheral on APB2. */
void rcc_apb2_periph_reset(uint32_t periph);

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_RCC_H */
