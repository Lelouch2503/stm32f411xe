/**
 * @file    rtos.h
 * @brief   Static-allocation mini-RTOS API for STM32F411 Cortex-M4F.
 */

#ifndef RTOS_H
#define RTOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "rtos_config.h"

typedef uint32_t RTOS_StackWord_t;
typedef void (*RTOS_TaskFunction_t)(void *argument);

typedef enum {
  RTOS_OK = 0,
  RTOS_ERR_INVALID = -1,
  RTOS_ERR_TIMEOUT = -2,
  RTOS_ERR_STATE = -3,
  RTOS_ERR_BUSY = -4,
  RTOS_ERR_NOT_OWNER = -5,
  RTOS_ERR_LIMIT = -6,
  RTOS_ERR_STACK = -7
} RTOS_Status_t;

typedef enum {
  RTOS_TASK_READY = 0,
  RTOS_TASK_RUNNING,
  RTOS_TASK_DELAYED,
  RTOS_TASK_BLOCKED,
  RTOS_TASK_TERMINATED
} RTOS_TaskState_t;

typedef enum {
  RTOS_FAULT_NONE = 0,
  RTOS_FAULT_STACK,
  RTOS_FAULT_KERNEL_INVARIANT,
  RTOS_FAULT_TASK_RETURN_WITH_MUTEX,
  RTOS_FAULT_HARDFAULT
} RTOS_FaultReason_t;

typedef struct {
  uint32_t tick_hz;                    /**< Scheduler tick frequency. */
  uint8_t max_syscall_irq_priority;    /**< Highest urgency API-safe IRQ. */
} RTOS_Config_t;

typedef struct RTOS_Task {
  /* The first two fields are part of the Cortex-M4 assembly ABI. */
  RTOS_StackWord_t *stack_pointer; /**< Saved PSP; assembly ABI offset 0. */
  uint32_t exc_return;             /**< Saved EXC_RETURN; ABI offset 4. */

  RTOS_TaskFunction_t function;    /**< Task entry function. */
  void *argument;                  /**< Task entry argument. */
  RTOS_StackWord_t *stack_base;    /**< Lowest caller-owned stack word. */
  uint32_t stack_words;            /**< Stack capacity in 32-bit words. */
  const char *name;                /**< Persistent debug name pointer. */
  void *wait_object;               /**< IPC object currently awaited. */
  void *wait_buffer;               /**< Queue send/receive buffer. */
  uint32_t wake_tick;              /**< Delay or timeout deadline. */
  uint32_t wait_sequence;          /**< FIFO tie-break sequence. */
  int wait_result;                 /**< Result supplied when unblocked. */
  RTOS_TaskState_t state;          /**< Current scheduler state. */
  uint8_t base_priority;           /**< Configured task priority. */
  uint8_t effective_priority;      /**< Priority after inheritance. */
  uint8_t wait_type;               /**< Internal wait-reason value. */
  uint8_t wait_forever;            /**< Nonzero for an infinite wait. */
  uint8_t mutex_hold_count;        /**< Number of currently owned mutexes. */
  uint8_t reserved[3];             /**< Maintains deterministic layout. */
} RTOS_Task_t;

typedef struct RTOS_Semaphore {
  uint32_t magic;                  /**< Initialization signature. */
  uint32_t count;                  /**< Available token count. */
  uint32_t maximum_count;          /**< Saturation limit. */
} RTOS_Semaphore_t;

typedef struct RTOS_Mutex {
  uint32_t magic;                  /**< Initialization signature. */
  RTOS_Task_t *owner;              /**< Current owner or NULL. */
} RTOS_Mutex_t;

typedef struct RTOS_Queue {
  uint32_t magic;                  /**< Initialization signature. */
  uint8_t *storage;                /**< Caller-owned item storage. */
  uint32_t capacity;               /**< Maximum number of items. */
  uint32_t item_size;              /**< Bytes copied per item. */
  uint32_t head;                   /**< Next read index. */
  uint32_t tail;                   /**< Next write index. */
  uint32_t count;                  /**< Current queued item count. */
} RTOS_Queue_t;

typedef struct {
  uint32_t signature;          /**< RTOS signature when record is valid. */
  RTOS_FaultReason_t reason;   /**< Kernel or processor fault reason. */
  const RTOS_Task_t *task;     /**< Task active when the fault occurred. */
  uint32_t psp;                /**< Process stack pointer snapshot. */
  uint32_t msp;                /**< Main stack pointer snapshot. */
  uint32_t exc_return;         /**< Exception-return code, if available. */
  uint32_t stacked_r0;         /**< Hardware-stacked R0. */
  uint32_t stacked_r1;         /**< Hardware-stacked R1. */
  uint32_t stacked_r2;         /**< Hardware-stacked R2. */
  uint32_t stacked_r3;         /**< Hardware-stacked R3. */
  uint32_t stacked_r12;        /**< Hardware-stacked R12. */
  uint32_t stacked_lr;         /**< Hardware-stacked LR. */
  uint32_t stacked_pc;         /**< Hardware-stacked PC. */
  uint32_t stacked_xpsr;       /**< Hardware-stacked xPSR. */
  uint32_t hfsr;               /**< HardFault Status Register. */
  uint32_t cfsr;               /**< Configurable Fault Status Register. */
  uint32_t mmfar;              /**< MemManage fault address. */
  uint32_t bfar;               /**< BusFault address. */
} RTOS_FaultRecord_t;

#define RTOS_STACK_DEFINE(name, words)                                      \
  static RTOS_StackWord_t name[(words)] __attribute__((aligned(8)))

extern volatile RTOS_FaultRecord_t rtos_fault_record;

/** Initialize the kernel before creating objects or tasks. */
int rtos_init(const RTOS_Config_t *config);

/** Create a task using caller-owned TCB and stack storage. */
int rtos_task_create_static(RTOS_Task_t *task,
                            RTOS_TaskFunction_t function,
                            void *argument,
                            RTOS_StackWord_t *stack,
                            uint32_t stack_words,
                            uint8_t priority,
                            const char *name);

/** Start scheduling. Returns only if startup validation fails. */
int rtos_start(void);

/** Yield the processor to another ready task. */
void rtos_task_yield(void);

/** Block the calling task for a number of ticks; zero behaves as yield. */
int rtos_task_delay(uint32_t ticks);

/** Return the shared 32-bit SysTick timebase. */
uint32_t rtos_tick_get(void);

/** Initialize a static counting semaphore. */
int rtos_semaphore_init(RTOS_Semaphore_t *semaphore,
                        uint32_t initial_count,
                        uint32_t maximum_count);

/** Take a semaphore, optionally blocking for timeout_ticks. */
int rtos_semaphore_take(RTOS_Semaphore_t *semaphore,
                        uint32_t timeout_ticks);

/** Give a semaphore from thread mode. */
int rtos_semaphore_give(RTOS_Semaphore_t *semaphore);

/** Give a semaphore from a kernel-aware interrupt. */
int rtos_semaphore_give_from_isr(RTOS_Semaphore_t *semaphore);

/** Initialize a non-recursive mutex. */
int rtos_mutex_init(RTOS_Mutex_t *mutex);

/** Lock a mutex, optionally blocking for timeout_ticks. */
int rtos_mutex_lock(RTOS_Mutex_t *mutex, uint32_t timeout_ticks);

/** Unlock a mutex owned by the calling task. */
int rtos_mutex_unlock(RTOS_Mutex_t *mutex);

/** Initialize a fixed-size queue using caller-owned storage. */
int rtos_queue_init(RTOS_Queue_t *queue,
                    void *storage,
                    uint32_t capacity,
                    uint32_t item_size);

/** Send one item to a queue, optionally blocking. */
int rtos_queue_send(RTOS_Queue_t *queue,
                    const void *item,
                    uint32_t timeout_ticks);

/** Receive one item from a queue, optionally blocking. */
int rtos_queue_receive(RTOS_Queue_t *queue,
                       void *item,
                       uint32_t timeout_ticks);

/** Send one item from a kernel-aware interrupt without blocking. */
int rtos_queue_send_from_isr(RTOS_Queue_t *queue, const void *item);

/** Receive one item from a kernel-aware interrupt without blocking. */
int rtos_queue_receive_from_isr(RTOS_Queue_t *queue, void *item);

/** User-overridable terminal fault hook. */
void rtos_fault_hook(RTOS_FaultReason_t reason, const RTOS_Task_t *task);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_H */
