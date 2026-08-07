/**
 * @file    main.c
 * @brief   Mini-RTOS demonstration for STM32F411E-DISCO.
 */

#include "rtos.h"
#include "stm32f411_gpio.h"
#include "stm32f411_nvic.h"
#include "stm32f411_rcc.h"
#include "stm32f411_usart.h"

#define DEMO_EVENT_QUEUE_LENGTH 16U
#define DEMO_TASK_STACK_WORDS   256U

typedef enum {
  DEMO_EVENT_GREEN = 1,
  DEMO_EVENT_BLUE,
  DEMO_EVENT_IRQ
} DemoEventId_t;

typedef struct {
  uint32_t event_id;
  uint32_t task_id;
  uint32_t tick;
} DemoEvent_t;

static RTOS_Task_t green_task_control;
static RTOS_Task_t blue_task_control;
static RTOS_Task_t irq_trigger_task_control;
static RTOS_Task_t irq_event_task_control;
static RTOS_Task_t logger_task_control;
static RTOS_Task_t diagnostics_task_control;

RTOS_STACK_DEFINE(green_task_stack, DEMO_TASK_STACK_WORDS);
RTOS_STACK_DEFINE(blue_task_stack, DEMO_TASK_STACK_WORDS);
RTOS_STACK_DEFINE(irq_trigger_task_stack, DEMO_TASK_STACK_WORDS);
RTOS_STACK_DEFINE(irq_event_task_stack, DEMO_TASK_STACK_WORDS);
RTOS_STACK_DEFINE(logger_task_stack, DEMO_TASK_STACK_WORDS);
RTOS_STACK_DEFINE(diagnostics_task_stack, DEMO_TASK_STACK_WORDS);

static RTOS_Semaphore_t irq_semaphore;
static RTOS_Mutex_t uart_mutex;
static RTOS_Queue_t event_queue;
static DemoEvent_t event_storage[DEMO_EVENT_QUEUE_LENGTH];
static volatile uint32_t dropped_log_count;

static void demo_fail(void) {
  (void)gpio_write_pin(DISCO_LED_PORT, DISCO_LED_ORANGE_PIN, GPIO_PIN_SET);
  while (1) {
  }
}

static void demo_send_event(DemoEventId_t event_id, uint32_t task_id) {
  DemoEvent_t event;
  event.event_id = (uint32_t)event_id;
  event.task_id = task_id;
  event.tick = rtos_tick_get();
  if (rtos_queue_send(&event_queue, &event, 0U) != RTOS_OK) {
    dropped_log_count++;
  }
}

static void demo_uart_write_u32(uint32_t value) {
  char buffer[11];
  uint32_t index = 0U;
  uint8_t output;

  do {
    buffer[index++] = (char)('0' + (value % 10U));
    value /= 10U;
  } while ((value != 0U) && (index < sizeof(buffer)));

  while (index != 0U) {
    index--;
    output = (uint8_t)buffer[index];
    (void)usart_transmit(USART2, &output, 1U, 0U);
  }
}

static void green_task(void *argument) {
  (void)argument;
  while (1) {
    (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_GREEN_PIN);
    demo_send_event(DEMO_EVENT_GREEN, 1U);
    (void)rtos_task_delay(250U);
  }
}

static void blue_task(void *argument) {
  (void)argument;
  while (1) {
    (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_BLUE_PIN);
    demo_send_event(DEMO_EVENT_BLUE, 2U);
    (void)rtos_task_delay(500U);
  }
}

static void irq_trigger_task(void *argument) {
  (void)argument;
  while (1) {
    (void)rtos_task_delay(1000U);
    (void)nvic_set_pending_irq(NVIC_IRQ_TAMP_STAMP);
  }
}

static void irq_event_task(void *argument) {
  (void)argument;
  while (1) {
    if (rtos_semaphore_take(&irq_semaphore, RTOS_WAIT_FOREVER) == RTOS_OK) {
      (void)gpio_toggle_pin(DISCO_LED_PORT, DISCO_LED_RED_PIN);
      demo_send_event(DEMO_EVENT_IRQ, 3U);
    }
  }
}

static void logger_task(void *argument) {
  DemoEvent_t event;
  const char *name;
  (void)argument;

  while (1) {
    if (rtos_queue_receive(&event_queue, &event, RTOS_WAIT_FOREVER) !=
        RTOS_OK) {
      continue;
    }
    if (event.event_id == DEMO_EVENT_GREEN) {
      name = "green";
    } else if (event.event_id == DEMO_EVENT_BLUE) {
      name = "blue";
    } else {
      name = "irq";
    }

    if (rtos_mutex_lock(&uart_mutex, RTOS_WAIT_FOREVER) == RTOS_OK) {
      (void)usart_puts(USART2, "event=", 0U);
      (void)usart_puts(USART2, name, 0U);
      (void)usart_puts(USART2, " task=", 0U);
      demo_uart_write_u32(event.task_id);
      (void)usart_puts(USART2, " tick=", 0U);
      demo_uart_write_u32(event.tick);
      (void)usart_puts(USART2, "\r\n", 0U);
      (void)rtos_mutex_unlock(&uart_mutex);
    }
  }
}

static void diagnostics_task(void *argument) {
  (void)argument;
  while (1) {
    (void)rtos_task_delay(2000U);
    if (rtos_mutex_lock(&uart_mutex, RTOS_WAIT_FOREVER) == RTOS_OK) {
      (void)usart_puts(USART2, "diag tick=", 0U);
      demo_uart_write_u32(rtos_tick_get());
      (void)usart_puts(USART2, " dropped=", 0U);
      demo_uart_write_u32(dropped_log_count);
      (void)usart_puts(USART2, "\r\n", 0U);
      (void)rtos_mutex_unlock(&uart_mutex);
    }
  }
}

void TAMP_STAMP_IRQHandler(void) {
  (void)rtos_semaphore_give_from_isr(&irq_semaphore);
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
  USART_Config_t uart_config = {
      .baudrate = 115200U,
      .word_length = USART_WORDLEN_8BIT,
      .parity = USART_PARITY_NONE,
      .stop_bits = USART_STOPBITS_1,
      .hw_flow_ctl = USART_HWFLOW_NONE,
      .mode = USART_MODE_TX_RX,
      .oversampling = USART_OVERSAMPLING_16,
  };
  RTOS_Config_t rtos_config = {
      .tick_hz = RTOS_DEFAULT_TICK_HZ,
      .max_syscall_irq_priority = RTOS_DEFAULT_MAX_SYSCALL_IRQ,
  };
  int rc;

  rc = rcc_sys_clk_config(&clock_config);
  if (gpio_disco_leds_init() != 0) {
    while (1) {
    }
  }
  if (rc != 0) {
    demo_fail();
  }
  if (usart_init(USART2, &uart_config) != 0) {
    demo_fail();
  }
  if (rtos_init(&rtos_config) != RTOS_OK) {
    demo_fail();
  }
  if ((rtos_semaphore_init(&irq_semaphore, 0U, 1U) != RTOS_OK) ||
      (rtos_mutex_init(&uart_mutex) != RTOS_OK) ||
      (rtos_queue_init(&event_queue, event_storage,
                       DEMO_EVENT_QUEUE_LENGTH,
                       sizeof(DemoEvent_t)) != RTOS_OK)) {
    demo_fail();
  }

  if ((rtos_task_create_static(&green_task_control, green_task, (void *)0,
                               green_task_stack, DEMO_TASK_STACK_WORDS, 2U,
                               "green") != RTOS_OK) ||
      (rtos_task_create_static(&blue_task_control, blue_task, (void *)0,
                               blue_task_stack, DEMO_TASK_STACK_WORDS, 2U,
                               "blue") != RTOS_OK) ||
      (rtos_task_create_static(&irq_trigger_task_control, irq_trigger_task,
                               (void *)0, irq_trigger_task_stack,
                               DEMO_TASK_STACK_WORDS, 1U,
                               "irq-trigger") != RTOS_OK) ||
      (rtos_task_create_static(&irq_event_task_control, irq_event_task,
                               (void *)0, irq_event_task_stack,
                               DEMO_TASK_STACK_WORDS, 4U,
                               "irq-event") != RTOS_OK) ||
      (rtos_task_create_static(&logger_task_control, logger_task, (void *)0,
                               logger_task_stack, DEMO_TASK_STACK_WORDS, 3U,
                               "logger") != RTOS_OK) ||
      (rtos_task_create_static(&diagnostics_task_control, diagnostics_task,
                               (void *)0, diagnostics_task_stack,
                               DEMO_TASK_STACK_WORDS, 1U,
                               "diagnostics") != RTOS_OK)) {
    demo_fail();
  }

  (void)nvic_set_priority(NVIC_IRQ_TAMP_STAMP, 6U);
  (void)nvic_clear_pending_irq(NVIC_IRQ_TAMP_STAMP);
  (void)nvic_enable_irq(NVIC_IRQ_TAMP_STAMP);
  (void)usart_puts(USART2,
                   "\r\n=== STM32F411E-DISCO mini-RTOS start ===\r\n",
                   0U);

  if (rtos_start() != RTOS_OK) {
    demo_fail();
  }
  demo_fail();
  return 0;
}
