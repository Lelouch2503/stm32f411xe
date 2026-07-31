/**
 * @file  main.c
 * @brief Bare-metal demo for STM32F411E-DISCO with GPIO, RCC, SysTick & USART
 *
 * Configures the system clock to 100 MHz via PLL (HSE 8 MHz),
 * initializes USART2 at 115200 baud (PA2 TX, PA3 RX), and
 * toggles user LEDs (PD12–PD15) while sending periodic heartbeats.
 * No HAL, no CMSIS — pure bare-metal.
 *
 * LED mapping (STM32F411E-DISCO):
 *   LD4 (Green)  → PD12
 *   LD3 (Orange) → PD13
 *   LD5 (Red)    → PD14
 *   LD6 (Blue)   → PD15
 *
 * USART2 wiring (external USB-to-UART adapter):
 *   PA2  → Adapter RX
 *   PA3  → Adapter TX
 *   GND  → Adapter GND
 */

#include "stm32f411_gpio.h"
#include "stm32f411_nvic.h"
#include "stm32f411_rcc.h"
#include "stm32f411_systick.h"
#include "stm32f411_usart.h"
#include "stm32f411_xe.h"

/* ══════════════════════════════════════════════════════════════════════
 * Interrupt Handlers
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Tamper and TimeStamp interrupt handler.
 * Overrides the weak alias defined in startup_stm32f411ve.s.
 */
void TAMP_STAMP_IRQHandler(void) {
  /* Toggle Red LED (PD14) via GPIO driver to signal ISR execution */
  (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_RED_PIN);
}

/* ---------- Main -------------------------------------------------- */
int main(void) {
  /* 0. Configure NVIC priority grouping and SysTick priority */
  nvic_set_priority_grouping(NVIC_PRIORITY_GROUP_4);
  nvic_set_priority(NVIC_IRQ_SYSTICK, 15U);

  /*
   * 1. Configure system clock: HSE 8 MHz → PLL → 100 MHz
   *
   *    VCO input  = HSE / PLLM = 8 / 4  = 2 MHz
   *    VCO output = VCO_in × PLLN = 2 × 200 = 400 MHz
   *    SYSCLK     = VCO / PLLP = 400 / 4 = 100 MHz
   *    USB/SDIO   = VCO / PLLQ = 400 / 8 =  50 MHz
   */
  RCC_ClkInit_t clk = {
      .sysclk_src = RCC_SYSCLK_PLL,
      .ahb_prescaler = RCC_AHB_DIV1,          /* HCLK  = 100 MHz */
      .apb1_prescaler = RCC_APB_DIV2,         /* PCLK1 =  50 MHz (APB1 max) */
      .apb2_prescaler = RCC_APB_DIV1,         /* PCLK2 = 100 MHz */
      .flash_latency = RCC_FLASH_LATENCY_3WS, /* 3 WS for 100 MHz @ 3.3 V */
      .pll =
          {
              .PLL_Source = RCC_PLLSRC_HSE,
              .PLLM = 4,
              .PLLN = 200,
              .PLLP = RCC_PLLP_DIV4,
              .PLLQ = 8,
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
      .clk_source = SYSTICK_CLKSRC_AHB, /* Full AHB = 100 MHz */
      .tick_freq_hz = 1000U,            /* 1 ms tick period   */
  };
  systick_init(&stk);

  /* 3. Initialize Discovery board user LEDs (PD12–PD15) via GPIO driver */
  (void)gpio_disco_leds_init();

  /* 4. Initialize Discovery board user button (PA0) via GPIO driver */
  (void)gpio_disco_button_init();

  /*
   * 5. Initialize USART2: 115200 baud, 8N1, TX+RX, 16x oversampling
   *    GPIO PA2 (TX) and PA3 (RX) are auto-configured by the driver.
   */
  USART_Config_t uart_cfg = {
      .baudrate = 115200U,
      .word_length = USART_WORDLEN_8BIT,
      .parity = USART_PARITY_NONE,
      .stop_bits = USART_STOPBITS_1,
      .hw_flow_ctl = USART_HWFLOW_NONE,
      .mode = USART_MODE_TX_RX,
      .oversampling = USART_OVERSAMPLING_16,
  };

  if (usart_init(USART2, &uart_cfg) != 0) {
    /* USART initialization failed — signal with Orange LED */
    (void)gpio_write_pin(DISCO_LED_PORT, DISCO_LED_ORANGE_PIN, GPIO_PIN_SET);
  }

  /* 6. Send startup banner over USART2 */
  (void)usart_puts(USART2,
                   "\r\n=== STM32F411E-DISCO Bare-Metal USART2 Ready ===\r\n",
                   0U);

  /* 7. Configure and Enable TAMP_STAMP interrupt in NVIC */
  nvic_set_priority(NVIC_IRQ_TAMP_STAMP, 10U);
  nvic_enable_irq(NVIC_IRQ_TAMP_STAMP);

  nvic_enable_irq(NVIC_IRQ_SYSTICK);
  nvic_set_priority(NVIC_IRQ_SYSTICK, 10U);

  /* 8. Main loop */
  while (1) {
    /* Toggle Green and Blue LEDs as main loop activity indicators */
    (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_GREEN_PIN);
    (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_BLUE_PIN);

    /* Send heartbeat over USART2 */
    (void)usart_puts(USART2, "heartbeat\r\n", 0U);

    /* Wait 500 ms */
    systick_delay(500);

    /* Trigger TAMP_STAMP interrupt via NVIC Software Pending flag */
    nvic_set_pending_irq(NVIC_IRQ_TAMP_STAMP);
  }

  /* Never reached */
  return 0;
}
