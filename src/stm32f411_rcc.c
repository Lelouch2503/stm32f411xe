/**
 * @file    stm32f411_rcc.c
 * @brief   RCC driver implementation for STM32F411xE
 *
 * Implements system clock configuration (HSI / HSE / PLL),
 * clock frequency readback, and peripheral clock gating.
 *
 * Reference: RM0383 Rev 3 — sections 5 (FLASH) and 6 (RCC)
 */

#include "stm32f411_rcc.h"

/* ══════════════════════════════════════════════════════════════════════
 * Private Constants
 * ═════════════════════════════════════════════════════════════════════ */

/** Maximum loop iterations to wait for oscillator / PLL ready flags */
#define RCC_TIMEOUT         0x5000U

/**
 * AHB prescaler decode table.
 * Indexed by the 4-bit HPRE field (RCC_CFGR[7:4]).
 *   0x0–0x7 → not divided (1)
 *   0x8     → /2  … 0xF → /512
 */
static const uint16_t ahb_prescaler_table[16] = {
    1, 1, 1, 1, 1, 1, 1, 1,            /* 0xxx: not divided */
    2, 4, 8, 16, 64, 128, 256, 512      /* 1xxx              */
};

/**
 * APB prescaler decode table.
 * Indexed by the 3-bit PPREx field (RCC_CFGR[12:10] or [15:13]).
 *   0x0–0x3 → not divided (1)
 *   0x4     → /2  … 0x7 → /16
 */
static const uint8_t apb_prescaler_table[8] = {
    1, 1, 1, 1, 2, 4, 8, 16            /* 0xx: not divided   */
};

/* ══════════════════════════════════════════════════════════════════════
 * System Clock Configuration
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Internal helper: set flash latency and enable caches.
 * @param  latency  Number of wait states (0–5).
 * @retval  0  Success
 * @retval -7  Flash latency verification failed
 */
static int rcc_set_flash_latency(uint32_t latency)
{
    uint32_t acr = FLASH->ACR;

    /* Clear latency bits, keep other settings */
    acr &= ~FLASH_ACR_LATENCY_Msk;

    /* Set new latency + enable prefetch, instruction cache, data cache */
    acr |= latency | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    FLASH->ACR = acr;

    /* Verify the latency was programmed correctly */
    if ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != latency) {
        return -7;
    }
    return 0;
}

/**
 * @brief  Internal helper: set AHB and APB prescalers in RCC_CFGR.
 */
static void rcc_set_prescalers(const RCC_ClkInit_t *cfg)
{
    uint32_t cfgr = RCC->CFGR;

    /* Clear prescaler fields */
    cfgr &= ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk);

    /* Set new prescaler values (shift field values into position) */
    cfgr |= ((uint32_t)cfg->ahb_prescaler  << RCC_CFGR_HPRE_Pos);
    cfgr |= ((uint32_t)cfg->apb1_prescaler << RCC_CFGR_PPRE1_Pos);
    cfgr |= ((uint32_t)cfg->apb2_prescaler << RCC_CFGR_PPRE2_Pos);

    RCC->CFGR = cfgr;
}

/**
 * @brief  Internal helper: switch system clock source and wait for confirmation.
 * @param  sw_value  One of RCC_CFGR_SW_HSI, RCC_CFGR_SW_HSE, RCC_CFGR_SW_PLL.
 * @retval  0  Success
 * @retval -8  Timeout waiting for SWS confirmation
 */
static int rcc_switch_sysclk(uint32_t sw_value)
{
    uint32_t cfgr = RCC->CFGR;

    /* Set SW bits */
    cfgr &= ~RCC_CFGR_SW_Msk;
    cfgr |= sw_value;
    RCC->CFGR = cfgr;

    /* Expected SWS value = SW shifted left by 2 */
    uint32_t expected_sws = sw_value << 2;

    uint32_t timeout = RCC_TIMEOUT;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != expected_sws) {
        if (--timeout == 0U) {
            return -8;
        }
    }
    return 0;
}

int rcc_sys_clk_config(const RCC_ClkInit_t *cfg)
{
    uint32_t timeout;
    int      rc;

    /* ── PLL Mode ───────────────────────────────────────────────────── */
    if (cfg->sysclk_src == RCC_SYSCLK_PLL) {

        /*
         * Step 1: If the system is already running from PLL,
         *         switch back to HSI before reconfiguring.
         */
        if ((RCC->CFGR & RCC_CFGR_SWS_Msk) == RCC_CFGR_SWS_PLL) {
            /* Make sure HSI is on */
            RCC->CR |= RCC_CR_HSION;
            timeout = RCC_TIMEOUT;
            while (!(RCC->CR & RCC_CR_HSIRDY)) {
                if (--timeout == 0U) return -1;
            }
            /* Switch to HSI */
            rc = rcc_switch_sysclk(RCC_CFGR_SW_HSI);
            if (rc != 0) return -2;
        }

        /* Step 2: Disable PLL before reconfiguring */
        RCC->CR &= ~RCC_CR_PLLON;
        timeout = RCC_TIMEOUT;
        while (RCC->CR & RCC_CR_PLLRDY) {
            if (--timeout == 0U) return -3;
        }

        /* Step 3: Enable PLL source oscillator */
        if (cfg->pll.PLL_Source == RCC_PLLSRC_HSE) {
            RCC->CR |= RCC_CR_HSEON;
            timeout = RCC_TIMEOUT;
            while (!(RCC->CR & RCC_CR_HSERDY)) {
                if (--timeout == 0U) return -4;
            }
        } else {
            RCC->CR |= RCC_CR_HSION;
            timeout = RCC_TIMEOUT;
            while (!(RCC->CR & RCC_CR_HSIRDY)) {
                if (--timeout == 0U) return -5;
            }
        }

        /*
         * Step 4: Enable PWR clock and set voltage regulator to Scale 1.
         *         Scale 1 supports up to 100 MHz (STM32F411 max).
         */
        RCC->APB1ENR |= RCC_APB1ENR_PWREN;
        (void)RCC->APB1ENR;    /* dummy read — bus sync delay */
        PWR->CR = (PWR->CR & ~PWR_CR_VOS_Msk) | PWR_CR_VOS_SCALE1;

        /* Step 5: Configure PLL (write the entire PLLCFGR register) */
        RCC->PLLCFGR = (cfg->pll.PLLM                        << RCC_PLLCFGR_PLLM_Pos)
                      | (cfg->pll.PLLN                        << RCC_PLLCFGR_PLLN_Pos)
                      | ((uint32_t)cfg->pll.PLLP              << RCC_PLLCFGR_PLLP_Pos)
                      | ((uint32_t)cfg->pll.PLL_Source        << RCC_PLLCFGR_PLLSRC_Pos)
                      | (cfg->pll.PLLQ                        << RCC_PLLCFGR_PLLQ_Pos);

        /* Step 6: Enable PLL and wait for lock */
        RCC->CR |= RCC_CR_PLLON;
        timeout = RCC_TIMEOUT;
        while (!(RCC->CR & RCC_CR_PLLRDY)) {
            if (--timeout == 0U) return -6;
        }

        /* Step 7: Set flash latency BEFORE switching to faster clock */
        rc = rcc_set_flash_latency((uint32_t)cfg->flash_latency);
        if (rc != 0) return rc;

        /* Step 8: Set bus prescalers */
        rcc_set_prescalers(cfg);

        /* Step 9: Switch system clock to PLL */
        rc = rcc_switch_sysclk(RCC_CFGR_SW_PLL);
        if (rc != 0) return rc;

    /* ── HSE Mode ───────────────────────────────────────────────────── */
    } else if (cfg->sysclk_src == RCC_SYSCLK_HSE) {

        /* Enable HSE */
        RCC->CR |= RCC_CR_HSEON;
        timeout = RCC_TIMEOUT;
        while (!(RCC->CR & RCC_CR_HSERDY)) {
            if (--timeout == 0U) return -4;
        }

        /* Flash latency and prescalers */
        rc = rcc_set_flash_latency((uint32_t)cfg->flash_latency);
        if (rc != 0) return rc;
        rcc_set_prescalers(cfg);

        /* Switch to HSE */
        rc = rcc_switch_sysclk(RCC_CFGR_SW_HSE);
        if (rc != 0) return rc;

    /* ── HSI Mode (default) ─────────────────────────────────────────── */
    } else {

        /* Enable HSI */
        RCC->CR |= RCC_CR_HSION;
        timeout = RCC_TIMEOUT;
        while (!(RCC->CR & RCC_CR_HSIRDY)) {
            if (--timeout == 0U) return -1;
        }

        /* Flash latency and prescalers */
        rc = rcc_set_flash_latency((uint32_t)cfg->flash_latency);
        if (rc != 0) return rc;
        rcc_set_prescalers(cfg);

        /* Switch to HSI */
        rc = rcc_switch_sysclk(RCC_CFGR_SW_HSI);
        if (rc != 0) return rc;
    }

    return 0;   /* Success */
}

/* ══════════════════════════════════════════════════════════════════════
 * Clock Frequency Readback
 * ═════════════════════════════════════════════════════════════════════ */

uint32_t rcc_get_sysclk_freq(void)
{
    uint32_t sws = RCC->CFGR & RCC_CFGR_SWS_Msk;

    switch (sws) {

    case RCC_CFGR_SWS_HSE:
        return HSE_VALUE;

    case RCC_CFGR_SWS_PLL: {
        uint32_t pllcfgr = RCC->PLLCFGR;

        uint32_t pllm = (pllcfgr & RCC_PLLCFGR_PLLM_Msk) >> RCC_PLLCFGR_PLLM_Pos;
        uint32_t plln = (pllcfgr & RCC_PLLCFGR_PLLN_Msk) >> RCC_PLLCFGR_PLLN_Pos;
        uint32_t pllp = (((pllcfgr & RCC_PLLCFGR_PLLP_Msk) >> RCC_PLLCFGR_PLLP_Pos) + 1U) * 2U;

        uint32_t pll_input = (pllcfgr & RCC_PLLCFGR_PLLSRC) ? HSE_VALUE : HSI_VALUE;

        /* SYSCLK = (pll_input / PLLM) × PLLN / PLLP */
        return (pll_input / pllm) * plln / pllp;
    }

    case RCC_CFGR_SWS_HSI:
    default:
        return HSI_VALUE;
    }
}

uint32_t rcc_get_hclk_freq(void)
{
    uint32_t hpre_idx = (RCC->CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos;
    return rcc_get_sysclk_freq() / ahb_prescaler_table[hpre_idx];
}

uint32_t rcc_get_pclk1_freq(void)
{
    uint32_t ppre1_idx = (RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos;
    return rcc_get_hclk_freq() / apb_prescaler_table[ppre1_idx];
}

uint32_t rcc_get_pclk2_freq(void)
{
    uint32_t ppre2_idx = (RCC->CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos;
    return rcc_get_hclk_freq() / apb_prescaler_table[ppre2_idx];
}

/* ══════════════════════════════════════════════════════════════════════
 * Peripheral Clock Enable / Disable
 * ═════════════════════════════════════════════════════════════════════ */

void rcc_ahb1_clk_enable(uint32_t periph)
{
    RCC->AHB1ENR |= periph;
    (void)RCC->AHB1ENR;    /* dummy read — ensures bus write completes (ST errata) */
}

void rcc_ahb1_clk_disable(uint32_t periph)
{
    RCC->AHB1ENR &= ~periph;
}

void rcc_apb1_clk_enable(uint32_t periph)
{
    RCC->APB1ENR |= periph;
    (void)RCC->APB1ENR;
}

void rcc_apb1_clk_disable(uint32_t periph)
{
    RCC->APB1ENR &= ~periph;
}

void rcc_apb2_clk_enable(uint32_t periph)
{
    RCC->APB2ENR |= periph;
    (void)RCC->APB2ENR;
}

void rcc_apb2_clk_disable(uint32_t periph)
{
    RCC->APB2ENR &= ~periph;
}

/* ══════════════════════════════════════════════════════════════════════
 * Peripheral Reset (assert + de-assert)
 * ═════════════════════════════════════════════════════════════════════ */

void rcc_ahb1_periph_reset(uint32_t periph)
{
    RCC->AHB1RSTR |=  periph;      /* assert reset  */
    __asm volatile ("dsb" ::: "memory");
    RCC->AHB1RSTR &= ~periph;      /* release reset */
}

void rcc_apb1_periph_reset(uint32_t periph)
{
    RCC->APB1RSTR |=  periph;
    __asm volatile ("dsb" ::: "memory");
    RCC->APB1RSTR &= ~periph;
}

void rcc_apb2_periph_reset(uint32_t periph)
{
    RCC->APB2RSTR |=  periph;
    __asm volatile ("dsb" ::: "memory");
    RCC->APB2RSTR &= ~periph;
}
