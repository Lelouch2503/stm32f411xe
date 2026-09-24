/**
 * @file    main.c
 * @brief   Three-task mini-RTOS demo using semaphores and I2C1.
 *
 * Task flow:
 *   trigger_task -> request_semaphore -> i2c_task
 *   i2c_task     -> done_semaphore    -> status_task
 *
 * I2C1 uses PB6 (SCL) and PB7 (SDA). External pull-up resistors are required.
 */

#include "rtos.h"
#include "stm32f411_gpio.h"
#include "stm32f411_i2c.h"
#include "stm32f411_rcc.h"

#define DEMO_TASK_STACK_WORDS     256U
#define I2C_SLAVE_ADDRESS         0x50U
#define I2C_TRANSFER_TIMEOUT      100000U
#define TRIGGER_PERIOD_TICKS      500U

static RTOS_Task_t trigger_task_control;
static RTOS_Task_t i2c_task_control;
static RTOS_Task_t status_task_control;

RTOS_STACK_DEFINE(trigger_task_stack, DEMO_TASK_STACK_WORDS);
RTOS_STACK_DEFINE(i2c_task_stack, DEMO_TASK_STACK_WORDS);
RTOS_STACK_DEFINE(status_task_stack, DEMO_TASK_STACK_WORDS);

static RTOS_Semaphore_t i2c_request_semaphore;
static RTOS_Semaphore_t i2c_done_semaphore;

static volatile int i2c_last_status;
static volatile uint8_t i2c_counter;

static void demo_fail(void) {
  (void)gpio_write_pin(DISCO_LED_PORT, DISCO_LED_ORANGE_PIN, GPIO_PIN_SET);
  while (1) {
  }
}

/**
 * @brief Periodically requests one I2C transfer.
 *
 * This task does not access I2C directly. It only signals the I2C task through
 * a semaphore, which keeps ownership of the peripheral in one task.
 */
static void trigger_task(void *argument) {
  (void)argument;

  while (1) {
    (void)rtos_task_delay(TRIGGER_PERIOD_TICKS);

    (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_BLUE_PIN);
    (void)rtos_semaphore_give(&i2c_request_semaphore);
  }
}

/**
 * @brief Owns I2C1 and performs the actual bus transfer.
 *
 * The payload matches the existing polling I2C example:
 *   0xA5, 0x5A, incrementing counter.
 */
static void i2c_task(void *argument) {
  uint8_t tx_data[3];

  (void)argument;

  while (1) {
    if (rtos_semaphore_take(&i2c_request_semaphore,
                            RTOS_WAIT_FOREVER) != RTOS_OK) {
      continue;
    }

    tx_data[0] = 0xA5U;
    tx_data[1] = 0x5AU;
    tx_data[2] = i2c_counter++;

    i2c_last_status =
        i2c_master_transmit(I2C1, I2C_SLAVE_ADDRESS, tx_data,
                            sizeof(tx_data), I2C_TRANSFER_TIMEOUT);

    (void)rtos_semaphore_give(&i2c_done_semaphore);
  }
}

/**
 * @brief Reports the result of each I2C transfer with the board LEDs.
 *
 * Green toggles after a successful transfer.
 * Red toggles after a failed transfer.
 */
static void status_task(void *argument) {
  (void)argument;

  while (1) {
    if (rtos_semaphore_take(&i2c_done_semaphore,
                            RTOS_WAIT_FOREVER) != RTOS_OK) {
      continue;
    }

    if (i2c_last_status == 0) {
      (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_GREEN_PIN);
    } else {
      (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_RED_PIN);
    }
  }
}

void rtos_fault_hook(RTOS_FaultReason_t reason, const RTOS_Task_t *task) {
  (void)reason;
  (void)task;

  (void)gpio_write_pin(DISCO_LED_PORT, DISCO_LED_ORANGE_PIN, GPIO_PIN_SET);
}

int main(void) {
  RCC_ClkInit_t clock_config = {
      .sysclk_src = RCC_SYSCLK_PLL,
      .ahb_prescaler = RCC_AHB_DIV1,
      .apb1_prescaler = RCC_APB_DIV2,
      .apb2_prescaler = RCC_APB_DIV1,
      .flash_latency = RCC_FLASH_LATENCY_3WS,
      .pll = {
          .PLL_Source = RCC_PLLSRC_HSE,
          .PLLM = 4,
          .PLLN = 200,
          .PLLP = RCC_PLLP_DIV4,
          .PLLQ = 8,
      },
  };

  I2C_Config_t i2c_config = {
      .clock_speed_hz = 100000U,
      .own_address = 0x00U,
      .duty_cycle = I2C_DUTY_CYCLE_2,
  };

  RTOS_Config_t rtos_config = {
      .tick_hz = RTOS_DEFAULT_TICK_HZ,
      .max_syscall_irq_priority = RTOS_DEFAULT_MAX_SYSCALL_IRQ,
  };

  if (rcc_sys_clk_config(&clock_config) != 0) {
    while (1) {
    }
  }

  if (gpio_disco_leds_init() != 0) {
    while (1) {
    }
  }

  /*
   * I2C1 pin mapping is configured by the driver:
   *   PB6 -> I2C1_SCL
   *   PB7 -> I2C1_SDA
   *
   * External pull-up resistors are required on SCL and SDA.
   */
  if (i2c_init(I2C1, &i2c_config) != 0) {
    demo_fail();
  }

  if (rtos_init(&rtos_config) != RTOS_OK) {
    demo_fail();
  }

  if ((rtos_semaphore_init(&i2c_request_semaphore, 0U, 1U) != RTOS_OK) ||
      (rtos_semaphore_init(&i2c_done_semaphore, 0U, 1U) != RTOS_OK)) {
    demo_fail();
  }

  /*
   * Priorities:
   *   i2c_task    = 3, highest: service the peripheral promptly.
   *   status_task = 2: consume completion notifications.
   *   trigger_task= 1: periodic producer.
   */
  if ((rtos_task_create_static(&trigger_task_control, trigger_task, (void *)0,
                               trigger_task_stack, DEMO_TASK_STACK_WORDS, 1U,
                               "trigger") != RTOS_OK) ||
      (rtos_task_create_static(&i2c_task_control, i2c_task, (void *)0,
                               i2c_task_stack, DEMO_TASK_STACK_WORDS, 3U,
                               "i2c") != RTOS_OK) ||
      (rtos_task_create_static(&status_task_control, status_task, (void *)0,
                               status_task_stack, DEMO_TASK_STACK_WORDS, 2U,
                               "status") != RTOS_OK)) {
    demo_fail();
  }

  if (rtos_start() != RTOS_OK) {
    demo_fail();
  }

  demo_fail();
  return 0;
}
