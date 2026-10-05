/**
 * @file    rtos_config.h
 * @brief   Compile-time configuration for the STM32F411 mini-RTOS.
 */

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

#include <stdint.h>

#define RTOS_MAX_TASKS                 8U
#define RTOS_PRIORITY_LEVELS           8U
#define RTOS_IDLE_PRIORITY             0U
#define RTOS_MIN_STACK_WORDS           128U
#define RTOS_IDLE_STACK_WORDS          128U
#define RTOS_QUEUE_MAX_ITEM_SIZE       64U
#define RTOS_DEFAULT_TICK_HZ           1000U
#define RTOS_DEFAULT_MAX_SYSCALL_IRQ   5U
#define RTOS_STACK_FILL_WORD           0xA5A5A5A5UL
#define RTOS_STACK_CANARY              0xDEADBEEFUL
#define RTOS_WAIT_FOREVER              UINT32_MAX

#endif /* RTOS_CONFIG_H */
