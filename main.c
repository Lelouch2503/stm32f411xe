#include "stm32f411_i2c.h"
#include "stm32f411_rcc.h"

#include <stdint.h>

#define I2C_ADDRESS 0x50U
#define I2C_TIMEOUT 100000u

volatile int i2c_status = 0;
volatile uint8_t counter = 0;

static void delay_ms(uint32_t ms) {
  while (ms-- != 0U) {
    for (volatile uint32_t i = 0U; i < 25000U; i++) {
      __asm volatile("nop");
    }
  }
}

static int system_clock_init(void) {
  RCC_ClkInit_t clock_cfg = {
      .sysclk_src = RCC_SYSCLK_PLL,
      .ahb_prescaler = RCC_AHB_DIV1,
      .apb1_prescaler = RCC_APB_DIV2,
      .apb2_prescaler = RCC_APB_DIV1,
      .flash_latency = RCC_FLASH_LATENCY_3WS,

      .pll =
          {
              .PLL_Source = RCC_PLLSRC_HSE,
              .PLLM = 4,
              .PLLN = 200,
              .PLLP = RCC_PLLP_DIV4,
              .PLLQ = 8,
          },
  };
  return rcc_sys_clk_config(&clock_cfg);
}

static int i2c_test_init(void) {
  I2C_Config_t config = {
      .clock_speed_hz = 100000U,
      .own_address = 0x00U,
      .duty_cycle = I2C_DUTY_CYCLE_2,
  };

  return i2c_init(I2C1, &config);
}

int main(void) {
  /*
   * Pattern chosen deliberately because it is
   * easy to recognize on the logic analyzer.
   */
  uint8_t tx_data[3];

  /*
   * ----------------------------------------------------------------
   * System clock
   * ----------------------------------------------------------------
   */

  if (system_clock_init() != 0) {
    /*
     * Clock initialization failed.
     */
    while (1) {
    }
  }

  /*
   * ----------------------------------------------------------------
   * I2C1 initialization
   * ----------------------------------------------------------------
   *
   * Driver automatically configures:
   *
   * PB6 -> AF4 -> I2C1_SCL
   * PB7 -> AF4 -> I2C1_SDA
   *
   * Open-drain
   * No internal pull-up
   */

  i2c_status = i2c_test_init();

  if (i2c_status != 0) {
    /*
     * Put breakpoint here if initialization fails.
     */
    while (1) {
    }
  }

  /*
   * Give the bus some time after initialization.
   */
  delay_ms(100U);

  /*
   * ----------------------------------------------------------------
   * I2C test loop
   * ----------------------------------------------------------------
   */
  while (1) {
    tx_data[0] = 0xA5U;
    tx_data[1] = 0x5AU;
    tx_data[2] = counter++;

    i2c_status = i2c_master_transmit(I2C1, I2C_ADDRESS, tx_data,
                                     sizeof(tx_data), I2C_TIMEOUT);
    delay_ms(500U);
  }
}