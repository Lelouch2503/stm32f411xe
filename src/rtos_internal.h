/**
 * @file    rtos_internal.h
 * @brief   Internal declarations and definitions for the STM32F411 mini-RTOS kernel.
 * @details Contains internal task state structures, wait type definitions, 
 *          kernel management functions, and hardware porting interfaces.
 */

#ifndef RTOS_INTERNAL_H
#define RTOS_INTERNAL_H

#include "rtos.h"
#include "stm32f411_systick.h"

/** @brief Magic signature for validating semaphore object structures. */
#define RTOS_SEMAPHORE_MAGIC 0x53454D41UL

/** @brief Magic signature for validating mutex object structures. */
#define RTOS_MUTEX_MAGIC     0x4D555458UL

/** @brief Magic signature for validating queue object structures. */
#define RTOS_QUEUE_MAGIC     0x51554555UL

/** @brief Cortex-M EXC_RETURN value for returning to Thread mode using PSP stack. */
#define RTOS_EXC_RETURN_THREAD_PSP 0xFFFFFFFDUL

/**
 * @brief Enumeration of task blocking/wait type categories.
 */
typedef enum {
  RTOS_WAIT_NONE = 0,        /**< Task is not waiting on any synchronization object. */
  RTOS_WAIT_SEMAPHORE,       /**< Task is blocked waiting to acquire a semaphore. */
  RTOS_WAIT_MUTEX,           /**< Task is blocked waiting to acquire a mutex. */
  RTOS_WAIT_QUEUE_SEND,      /**< Task is blocked waiting for space to send to a queue. */
  RTOS_WAIT_QUEUE_RECEIVE    /**< Task is blocked waiting to receive from a queue. */
} RTOS_WaitType_t;

/** @brief Global pointer to the task control block (TCB) of the currently executing task. */
extern RTOS_Task_t *volatile rtos_current_task;

/** @brief Basepri mask corresponding to maximum syscall priority level (config.max_syscall_irq_priority << 4). */
extern uint32_t rtos_max_syscall_priority_mask;

/* ============================================================================
 * Architecture Porting API Prototypes
 * ============================================================================ */

/**
 * @brief  Enters a kernel critical section by setting BASEPRI to max_syscall_priority_mask.
 * @return Previous BASEPRI register value prior to entering critical section.
 */
uint32_t rtos_port_enter_critical(void);

/**
 * @brief  Exits a kernel critical section by restoring the BASEPRI register value.
 * @param  previous_basepri BASEPRI value saved prior to entering critical section.
 */
void rtos_port_exit_critical(uint32_t previous_basepri);

/**
 * @brief  Reads the Interrupt Program Status Register (IPSR).
 * @return IPSR value (0 in Thread mode, non-zero ISR vector number in handler mode).
 */
uint32_t rtos_port_get_ipsr(void);

/**
 * @brief  Reads the Process Stack Pointer (PSP).
 * @return Current value of the PSP register.
 */
uint32_t rtos_port_get_psp(void);

/**
 * @brief  Reads the Main Stack Pointer (MSP).
 * @return Current value of the MSP register.
 */
uint32_t rtos_port_get_msp(void);

/**
 * @brief  Determines whether the currently executing interrupt is kernel-aware.
 * @return 1 if executing in a kernel-aware ISR (priority >= max syscall priority), 0 otherwise.
 */
int rtos_port_is_kernel_aware_isr(void);

/**
 * @brief  Triggers a PendSV exception to request an asynchronous context switch.
 */
void rtos_port_pend_context_switch(void);

/**
 * @brief  Executes an SVC instruction to start execution of the first task.
 */
void rtos_port_start_first_task(void);

/**
 * @brief  Executes WFI instruction to enter low-power idle state until next interrupt.
 */
void rtos_port_wait_for_interrupt(void);

/* ============================================================================
 * Internal Kernel Management Functions
 * ============================================================================ */

/**
 * @brief  Selects the next task to run; invoked from the PendSV handler assembly.
 */
void rtos_schedule_from_pendsv(void);

/**
 * @brief  System tick handler invoked on every SysTick timer interrupt.
 */
void rtos_tick_isr(void);

/**
 * @brief  Starts the SysTick timer when SVC exception handler executes during startup.
 */
void rtos_start_tick_from_svc(void);

/**
 * @brief  Task entry wrapper trampoline function. Handles initial task call and return cleanup.
 * @param  task Pointer to the task control block.
 */
void rtos_task_entry_trampoline(RTOS_Task_t *task);

/**
 * @brief  Captures CPU exception frame registers and stores diagnostic info on HardFault.
 * @param  frame Pointer to stacked registers (R0-R3, R12, LR, PC, xPSR).
 * @param  exc_return EXC_RETURN code in LR when HardFault occurred.
 */
void rtos_hardfault_capture(uint32_t *frame, uint32_t exc_return);

/**
 * @brief  Triggers a fatal kernel fault, saving diagnostic data and halting system execution.
 * @param  reason Reason code explaining the kernel failure.
 * @param  task Pointer to task TCB associated with the fault (or NULL).
 */
void rtos_kernel_fault(RTOS_FaultReason_t reason, const RTOS_Task_t *task);

/**
 * @brief  Blocks current task, placing it on wait state for an object or timeout.
 * @param  wait_type Reason category for blocking (semaphore, mutex, queue, etc.).
 * @param  object Pointer to blocking synchronization object.
 * @param  buffer Buffer pointer used for queue operations (or NULL).
 * @param  timeout_ticks Duration to wait in ticks (or RTOS_WAIT_FOREVER).
 * @return RTOS_OK on success, or error code on invalid configuration.
 */
int rtos_kernel_block_current(RTOS_WaitType_t wait_type,
                              void *object,
                              void *buffer,
                              uint32_t timeout_ticks);

/**
 * @brief  Finds the highest priority task waiting on a specific object.
 * @param  wait_type Wait category to search for.
 * @param  object Pointer to synchronization object.
 * @return Pointer to highest priority task TCB waiting on object, or NULL if none.
 */
RTOS_Task_t *rtos_kernel_find_waiter(RTOS_WaitType_t wait_type,
                                     const void *object);

/**
 * @brief  Unblocks a task and sets its wait result status.
 * @param  task Pointer to task TCB to wake up.
 * @param  result Status code passed to woken task (e.g. RTOS_OK or RTOS_ERR_TIMEOUT).
 */
void rtos_kernel_wake_task(RTOS_Task_t *task, int result);

/**
 * @brief  Recalculates task effective priorities to enforce priority inheritance for mutexes.
 */
void rtos_kernel_recompute_priorities(void);

/**
 * @brief  Validates if current context permits executing thread-level blocking APIs.
 * @return 1 if kernel is running and executing in Thread mode, 0 otherwise.
 */
int rtos_kernel_thread_api_valid(void);

/**
 * @brief  Checks whether kernel initialization (rtos_init) has completed.
 * @return 1 if initialized, 0 otherwise.
 */
int rtos_kernel_is_initialized(void);

/**
 * @brief  Checks whether kernel scheduler is active (rtos_start called).
 * @return 1 if running, 0 otherwise.
 */
int rtos_kernel_is_running(void);

/**
 * @brief  Determines if target task priority is strictly higher than current task priority.
 * @param  task Pointer to candidate task TCB.
 * @return 1 if candidate should preempt current task, 0 otherwise.
 */
int rtos_kernel_should_preempt(const RTOS_Task_t *task);

/* ============================================================================
 * Internal SysTick Drivers Helper Prototypes
 * ============================================================================ */

/**
 * @brief  Configures SysTick timer without immediately enabling its interrupt.
 * @param  cfg Pointer to SysTick configuration parameters.
 * @return 0 on success, negative error code on failure.
 */
int systick_configure_stopped(const SysTick_Config_t *cfg);

/**
 * @brief  Enables SysTick peripheral timer and interrupt.
 */
void systick_start_internal(void);

/**
 * @brief  Registers callback function hook executed on SysTick interrupt.
 * @param  hook Function pointer to tick callback routine.
 * @return 0 on success, negative error code on failure.
 */
int systick_register_kernel_hook(void (*hook)(void));

#ifdef RTOS_HOST_TEST
/** @brief Resets RTOS kernel global state variables during host unit testing. */
void rtos_test_reset(void);
/** @brief Sets current tick count value for unit test environment. */
void rtos_test_set_tick(uint32_t tick);
/** @brief Manually triggers tick ISR handling in unit test environment. */
void rtos_test_tick(void);
/** @brief Invokes task scheduler selection in host test environment. */
RTOS_Task_t *rtos_test_select_next(void);
/** @brief Forces specified task as current running task in host test environment. */
void rtos_test_set_running(RTOS_Task_t *task);
#endif

#endif /* RTOS_INTERNAL_H */

