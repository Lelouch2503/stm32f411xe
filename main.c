/**
 * @file  main.c
 * @brief Bare-metal LED blink for STM32F411E-DISCO with RCC + SysTick drivers
 *
 * Configures the system clock to 100 MHz via PLL (HSE 8 MHz)
 * and toggles all 4 user LEDs (PD12–PD15).
 * No HAL, no CMSIS — pure bare-metal.
 *
 * LED mapping (STM32F411E-DISCO):
 *   LD4 (Green)  → PD12
 *   LD3 (Orange) → PD13
 *   LD5 (Red)    → PD14
 *   LD6 (Blue)   → PD15
 */

#include "stm32f411_xe.h"
#include "stm32f411_rcc.h"
#include "stm32f411_systick.h"

/* ---------- GPIOD raw register access (until GPIO driver exists) --- */
#define GPIOD_MODER         (*(volatile uint32_t *)(GPIOD_BASE + 0x00U))
#define GPIOD_ODR           (*(volatile uint32_t *)(GPIOD_BASE + 0x14U))

/* ---------- Pin definitions --------------------------------------- */
#define LED_GREEN            12U   /* PD12 */
#define LED_ORANGE           13U   /* PD13 */
#define LED_RED              14U   /* PD14 */
#define LED_BLUE             15U   /* PD15 */

#define LED_ALL_MASK         ((1U << LED_GREEN)  | \
                              (1U << LED_ORANGE) | \
                              (1U << LED_RED)    | \
                              (1U << LED_BLUE))



/* ---------- Main -------------------------------------------------- */
int main(void)
{
    /*
     * 1. Configure system clock: HSE 8 MHz → PLL → 100 MHz
     *
     *    VCO input  = HSE / PLLM = 8 / 4  = 2 MHz
     *    VCO output = VCO_in × PLLN = 2 × 200 = 400 MHz
     *    SYSCLK     = VCO / PLLP = 400 / 4 = 100 MHz
     *    USB/SDIO   = VCO / PLLQ = 400 / 8 =  50 MHz
     */
    RCC_ClkInit_t clk = {
        .sysclk_src     = RCC_SYSCLK_PLL,
        .ahb_prescaler  = RCC_AHB_DIV1,          /* HCLK  = 100 MHz */
        .apb1_prescaler = RCC_APB_DIV2,           /* PCLK1 =  50 MHz (APB1 max) */
        .apb2_prescaler = RCC_APB_DIV1,           /* PCLK2 = 100 MHz */
        .flash_latency  = RCC_FLASH_LATENCY_3WS,  /* 3 WS for 100 MHz @ 3.3 V */
        .pll = {
            .PLL_Source = RCC_PLLSRC_HSE,
            .PLLM      = 4,
            .PLLN      = 200,
            .PLLP      = RCC_PLLP_DIV4,
            .PLLQ      = 8,
        },
    };

    if (rcc_sys_clk_config(&clk) != 0) {
        /* Clock configuration failed — blink continues at HSI 16 MHz */
    }

    /*
     * 2. Configure SysTick: 1 kHz tick (1 ms period) using AHB clock.
     *    The driver reads HCLK from rcc_get_hclk_freq() automatically.
     */
    SysTick_Config_t stk = {
        .clk_source  = SYSTICK_CLKSRC_AHB,   /* Full AHB = 100 MHz */
        .tick_freq_hz = 1000U,                /* 1 ms tick period   */
    };
    systick_init(&stk);

    /* 3. Enable GPIOD clock via RCC driver */
    rcc_ahb1_clk_enable(RCC_AHB1ENR_GPIODEN);

    /* 4. Configure PD12–PD15 as General Purpose Output (push-pull)
     *    MODER bits: 01 = General purpose output mode
     *    Each pin uses 2 bits in MODER register:
     *      PD12 → bits [25:24]
     *      PD13 → bits [27:26]
     *      PD14 → bits [29:28]
     *      PD15 → bits [31:30]
     */
    /* Clear mode bits for PD12–PD15 */
    GPIOD_MODER &= ~((3U << (LED_GREEN  * 2)) |
                      (3U << (LED_ORANGE * 2)) |
                      (3U << (LED_RED    * 2)) |
                      (3U << (LED_BLUE   * 2)));

    /* Set mode to output (01) for PD12–PD15 */
    GPIOD_MODER |=  ((1U << (LED_GREEN  * 2)) |
                      (1U << (LED_ORANGE * 2)) |
                      (1U << (LED_RED    * 2)) |
                      (1U << (LED_BLUE   * 2)));

    /* 5. Blink loop */
    while (1) {
        /* Toggle all LEDs via XOR on ODR */
        GPIOD_ODR ^= LED_ALL_MASK;

        /* Accurate 500 ms delay using SysTick */
        systick_delay(500);
    }

    /* Never reached */
    return 0;
}
