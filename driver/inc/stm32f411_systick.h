/**
 * @file    stm32f411_systick.h
 * @brief   SysTick driver for STM32F411xE — public API
 *
 * Provides functions to:
 *  - Configure SysTick timer with selectable clock source and tick frequency
 *  - Read the millisecond tick counter
 *  - Perform blocking delays with accurate timing
 *  - Check timeout conditions in peripheral drivers (I2C, SPI, UART…)
 *
 * The driver uses the Cortex-M4 SysTick timer and automatically reads
 * the current HCLK frequency from the RCC driver to compute the correct
 * reload value.
 *
 * Usage pattern for timeout in peripheral drivers:
 * @code
 *   uint32_t start = systick_get_tick();
 *   while (!(PERIPH->SR & FLAG)) {
 *       if (systick_is_timeout(start, 100)) {   // 100 ms timeout
 *           return -1;  // timeout error
 *       }
 *   }
 * @endcode
 *
 * Reference: ARMv7-M Architecture Reference Manual §B3.3
 */

#ifndef STM32F411_SYSTICK_H
#define STM32F411_SYSTICK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f411_xe.h"

/* ══════════════════════════════════════════════════════════════════════
 * Enumerations — descriptive values for user clarity
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * SysTick clock source selection (maps to CTRL CLKSOURCE bit).
 *
 * The clock source determines the counting speed:
 *  - AHB:     Full processor clock (e.g. 100 MHz)
 *  - AHB/8:   Processor clock divided by 8 (e.g. 12.5 MHz)
 *
 * Using AHB/8 allows longer tick periods within the 24-bit reload limit,
 * but reduces timing resolution.
 */
typedef enum {
  SYSTICK_CLKSRC_AHB_DIV8 = 0U,  /**< External ref clock = AHB / 8     */
  SYSTICK_CLKSRC_AHB      = 1U   /**< Processor clock    = AHB         */
} SysTick_ClkSrc_t;

/* ══════════════════════════════════════════════════════════════════════
 * Configuration Structure
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief SysTick configuration parameters.
 *
 * Fill in this struct and pass to systick_init().
 * The driver will read HCLK from the RCC driver and compute the
 * correct RELOAD value automatically.
 *
 * Example — 1 kHz tick (1 ms period) using full AHB clock:
 * @code
 *   SysTick_Config_t stk = {
 *       .clk_source  = SYSTICK_CLKSRC_AHB,
 *       .tick_freq_hz = 1000U,   // 1 ms tick
 *   };
 *   systick_init(&stk);
 * @endcode
 *
 * Constraints:
 *  - tick_freq_hz must be > 0
 *  - The computed RELOAD value (clk / tick_freq_hz - 1) must fit
 *    in 24 bits (≤ 16,777,215). If it exceeds, systick_init() returns -1.
 *  - Minimum practical tick_freq_hz depends on clock source:
 *    AHB 100 MHz → min ~6 Hz;  AHB/8 12.5 MHz → min ~1 Hz
 */
typedef struct {
  SysTick_ClkSrc_t clk_source;   /**< Clock source: AHB or AHB/8       */
  uint32_t         tick_freq_hz;  /**< Desired tick frequency in Hz
                                       (e.g. 1000 for 1ms period)       */
} SysTick_Config_t;

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Initialization
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Configure and start the SysTick timer.
 *
 * Reads the current HCLK frequency via rcc_get_hclk_freq(),
 * applies the clock source divider, computes the RELOAD value,
 * and enables the SysTick interrupt.
 *
 * Must be called AFTER rcc_sys_clk_config() has configured the
 * system clock, otherwise the RELOAD value will be incorrect.
 *
 * @param  cfg  Pointer to a filled-in SysTick_Config_t.
 * @retval  0   Success
 * @retval -1   Invalid config (tick_freq_hz is 0 or RELOAD > 24-bit)
 */
int systick_init(const SysTick_Config_t *cfg);

/**
 * @brief  Reconfigure SysTick after a runtime HCLK change.
 *
 * Re-reads HCLK from RCC driver and recomputes the RELOAD value
 * using the same config that was passed to systick_init().
 *
 * Call this after any runtime system clock reconfiguration.
 *
 * @retval  0   Success
 * @retval -1   RELOAD exceeds 24-bit limit at new HCLK
 * @retval -2   systick_init() was never called
 */
int systick_update_freq(void);

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Tick Counter
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Return the number of ticks elapsed since systick_init().
 *
 * At tick_freq_hz = 1000, this returns milliseconds.
 * The counter wraps around after ~49.7 days (2^32 ms).
 */
uint32_t systick_get_tick(void);

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Blocking Delay
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Blocking delay for the specified number of ticks.
 *
 * At tick_freq_hz = 1000, this delays in milliseconds.
 * Uses the SysTick interrupt counter — NOT a busy-wait NOP loop.
 * Handles uint32_t wrap-around correctly.
 *
 * @param  ticks  Number of ticks to wait
 */
void systick_delay(uint32_t ticks);

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Timeout Utilities (for peripheral drivers)
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Check whether a timeout has expired.
 *
 * Designed for use inside peripheral driver polling loops.
 * Uses unsigned subtraction — correct even when the tick counter
 * wraps around from 0xFFFFFFFF → 0x00000000.
 *
 * @param  start_tick   Tick value captured at the start of the wait
 *                      (from systick_get_tick())
 * @param  timeout      Maximum number of ticks to wait
 * @retval 1  Timeout has expired
 * @retval 0  Timeout has NOT expired
 */
int systick_is_timeout(uint32_t start_tick, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_SYSTICK_H */
