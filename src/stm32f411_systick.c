/**
 * @file    stm32f411_systick.c
 * @brief   SysTick driver implementation for STM32F411xE
 *
 * Implements the Cortex-M4 SysTick timer as a system timebase.
 * The SysTick_Handler ISR increments a volatile tick counter every
 * time the counter reaches zero.
 *
 * The driver reads the actual HCLK frequency from the RCC driver
 * to compute the correct RELOAD value, making it independent of
 * any hardcoded clock assumptions.
 *
 * Reference: ARMv7-M Architecture Reference Manual §B3.3
 */

#include "stm32f411_systick.h"
#include "stm32f411_rcc.h"

/* ══════════════════════════════════════════════════════════════════════
 * Private State
 * ═════════════════════════════════════════════════════════════════════ */

/** Tick counter — incremented by SysTick_Handler every tick period */
static volatile uint32_t systick_ticks = 0U;

/**
 * Saved configuration from the last systick_init() call.
 * Used by systick_update_freq() to recalculate RELOAD after
 * a runtime HCLK change without requiring the user to pass
 * the config struct again.
 */
static SysTick_Config_t systick_saved_cfg;

/** Flag: has systick_init() been called at least once? */
static uint8_t systick_initialized = 0U;

/* ══════════════════════════════════════════════════════════════════════
 * Private Helpers
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Compute and apply the SysTick RELOAD value.
 *
 * @param  cfg  Pointer to configuration
 * @retval  0   Success
 * @retval -1   Invalid config or RELOAD exceeds 24-bit limit
 */
static int systick_configure(const SysTick_Config_t *cfg) {
  /* Validate tick frequency */
  if (cfg->tick_freq_hz == 0U) {
    return -1;
  }

  /* Get current HCLK from RCC driver */
  uint32_t clk_hz = rcc_get_hclk_freq();

  /* Apply clock source divider */
  if (cfg->clk_source == SYSTICK_CLKSRC_AHB_DIV8) {
    clk_hz /= 8U;
  }

  /* Compute reload value: RELOAD = (clk / freq) - 1 */
  uint32_t reload = (clk_hz / cfg->tick_freq_hz) - 1U;

  /* Validate: RELOAD must fit in 24 bits */
  if (reload > SYSTICK_LOAD_RELOAD_Msk) {
    return -1;
  }

  /* ── Configure SysTick registers ──────────────────────────────── */

  /* 1. Disable SysTick during configuration */
  SYSTICK->CTRL.reg = 0U;

  /* 2. Set reload value */
  SYSTICK->LOAD.reg = reload & SYSTICK_LOAD_RELOAD_Msk;

  /* 3. Clear current value (writing any value to VAL clears it) */
  SYSTICK->VAL.reg = 0U;

  /* 4. Configure clock source, interrupt, and enable counting */
  uint32_t ctrl = SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
  if (cfg->clk_source == SYSTICK_CLKSRC_AHB) {
    ctrl |= SYSTICK_CTRL_CLKSOURCE;
  }
  SYSTICK->CTRL.reg = ctrl;

  return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * SysTick Interrupt Handler
 *
 * Overrides the weak alias defined in startup_stm32f411ve.s.
 * COUNTFLAG is automatically cleared when CTRL is read (which the
 * hardware does as part of exception entry), so no explicit clear
 * is needed.
 * ═════════════════════════════════════════════════════════════════════ */

void SysTick_Handler(void) { systick_ticks++; }

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Initialization
 * ═════════════════════════════════════════════════════════════════════ */

int systick_init(const SysTick_Config_t *cfg) {
  int rc = systick_configure(cfg);
  if (rc != 0) {
    return rc;
  }

  /* Save config for later use by systick_update_freq() */
  systick_saved_cfg = *cfg;
  systick_initialized = 1U;

  /* Reset tick counter */
  systick_ticks = 0U;

  return 0;
}

int systick_update_freq(void) {
  if (!systick_initialized) {
    return -2;
  }

  return systick_configure(&systick_saved_cfg);
}

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Tick Counter
 * ═════════════════════════════════════════════════════════════════════ */

uint32_t systick_get_tick(void) { return systick_ticks; }

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Blocking Delay
 * ═════════════════════════════════════════════════════════════════════ */

void systick_delay(uint32_t ticks) {
  uint32_t start = systick_get_tick();

  /* Unsigned subtraction handles wrap-around correctly:
   *   (current - start) gives the correct elapsed count
   *   even when current < start (counter wrapped).
   *
   * At 1 kHz tick rate, wraps after ~49.7 days.
   */
  while ((systick_get_tick() - start) < ticks) {
    /* Wait — the SysTick ISR keeps incrementing systick_ticks */
  }
}

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Timeout Utilities
 * ═════════════════════════════════════════════════════════════════════ */

int systick_is_timeout(uint32_t start_tick, uint32_t timeout) {
  return ((systick_get_tick() - start_tick) >= timeout) ? 1 : 0;
}
