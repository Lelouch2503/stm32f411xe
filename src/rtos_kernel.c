/**
 * @file    rtos_kernel.c
 * @brief   Fixed-priority preemptive scheduler for the STM32F411 mini-RTOS.
 */

#include "rtos_internal.h"

#ifndef RTOS_HOST_TEST
#include "stm32f411_nvic.h"
#include "stm32f411_xe.h"
#endif

/** @brief Magic signature written to fault records to verify valid diagnostic records. */
#define RTOS_FAULT_SIGNATURE 0x52544F53UL

/** @brief Global pointer to the TCB of the currently active/running task. */
RTOS_Task_t *volatile rtos_current_task = (RTOS_Task_t *)0;

/** @brief Basepri mask corresponding to maximum syscall IRQ priority. Default: priority 5 (0x50). */
uint32_t rtos_max_syscall_priority_mask =
    (RTOS_DEFAULT_MAX_SYSCALL_IRQ << 4U);

/** @brief Fault record structure storing diagnostic information on HardFault or kernel panic. */
volatile RTOS_FaultRecord_t rtos_fault_record;

/** @brief Array of pointers to registered task control blocks (including idle task). */
static RTOS_Task_t *rtos_tasks[RTOS_MAX_TASKS + 1U];

/** @brief Total count of registered tasks (application tasks + idle task). */
static uint32_t rtos_task_count;

/** @brief Total count of user/application tasks created (excluding idle task). */
static uint32_t rtos_application_task_count;

/** @brief Global sequence counter to preserve FIFO ordering for equal priority waiting tasks. */
static uint32_t rtos_wait_sequence;

/** @brief Round-robin rotation cursor index per priority level. */
static uint8_t rtos_rr_cursor[RTOS_PRIORITY_LEVELS];

/** @brief Flag indicating if rtos_init() has been executed. */
static uint8_t rtos_initialized;

/** @brief Flag indicating if rtos_start() has been executed and scheduler is running. */
static uint8_t rtos_running;

/** @brief Stored RTOS configuration parameters. */
static RTOS_Config_t rtos_config;

/** @brief Task Control Block for the default system idle task. */
static RTOS_Task_t rtos_idle_task;

/** @brief Stack allocation buffer for the default system idle task. */
RTOS_STACK_DEFINE(rtos_idle_stack, RTOS_IDLE_STACK_WORDS);

/**
 * @brief  Entry function for the low-priority system idle task.
 * @param  argument Unused argument pointer.
 */
static void rtos_idle_entry(void *argument) {
  (void)argument;
  while (1) {
    /* Put CPU to sleep until next interrupt fires */
    rtos_port_wait_for_interrupt();
  }
}

/**
 * @brief  Validates whether a timeout tick value is within allowable limits.
 * @param  timeout_ticks Timeout value in ticks.
 * @return 1 if valid, 0 if out of range.
 */
static int rtos_timeout_valid(uint32_t timeout_ticks) {
  return (timeout_ticks == RTOS_WAIT_FOREVER) ||
         (timeout_ticks <= (uint32_t)INT32_MAX);
}

/**
 * @brief  Checks if current tick timestamp has met or passed a deadline timestamp.
 * @param  now Current SysTick count.
 * @param  deadline Target wake-up SysTick timestamp.
 * @return 1 if deadline reached or passed, 0 otherwise.
 */
static int rtos_deadline_reached(uint32_t now, uint32_t deadline) {
  return ((int32_t)(now - deadline) >= 0) ? 1 : 0;
}

/**
 * @brief  Validates stack pointer alignment, stack bounds, and canary integrity.
 * @param  task Pointer to task TCB to check.
 * @return 1 if stack is valid, 0 if corrupted or invalid.
 */
static int rtos_task_stack_valid(const RTOS_Task_t *task) {
  uintptr_t low;
  uintptr_t high;
  uintptr_t saved;

  if ((task == (const RTOS_Task_t *)0) ||
      (task->stack_base == (RTOS_StackWord_t *)0) ||
      (task->stack_words < RTOS_MIN_STACK_WORDS)) {
    return 0;
  }

  low = (uintptr_t)&task->stack_base[1];
  high = (uintptr_t)&task->stack_base[task->stack_words];
  saved = (uintptr_t)task->stack_pointer;

  /* Check stack canary at stack_base[0], bounds, and 8-byte stack pointer alignment */
  if ((task->stack_base[0] != RTOS_STACK_CANARY) ||
      (saved < low) || (saved > high) || ((saved & 7U) != 0U)) {
    return 0;
  }
  return 1;
}

/**
 * @brief  Initializes task stack frame layout for Cortex-M hardware auto-stacking and context switch.
 * @param  task Pointer to target task TCB.
 */
static void rtos_stack_initialize(RTOS_Task_t *task) {
  RTOS_StackWord_t *sp;
  uint32_t index;

  /* Fill stack space with fill pattern and place stack canary at index 0 */
  for (index = 0U; index < task->stack_words; index++) {
    task->stack_base[index] = RTOS_STACK_FILL_WORD;
  }
  task->stack_base[0] = RTOS_STACK_CANARY;

  /* Set stack pointer to top of stack buffer (8-byte aligned) */
  sp = &task->stack_base[task->stack_words];

  /* Hardware auto-saved context frame (xPSR, PC, LR, R12, R3, R2, R1, R0) */
  *(--sp) = 0x01000000UL; /* xPSR: Thumb state bit set. */
  *(--sp) = (uint32_t)(uintptr_t)rtos_task_entry_trampoline;
  *(--sp) = 0U;          /* LR unused by trampoline. */
  *(--sp) = 0U;          /* R12 */
  *(--sp) = 0U;          /* R3  */
  *(--sp) = 0U;          /* R2  */
  *(--sp) = 0U;          /* R1  */
  *(--sp) = (uint32_t)(uintptr_t)task; /* R0: passed as argument to trampoline */

  /* Software saved context frame (R4-R11) */
  for (index = 0U; index < 8U; index++) {
    *(--sp) = 0U;        /* R4-R11 initialized to 0, restored by PendSV/SVC. */
  }

  task->stack_pointer = sp;
  task->exc_return = RTOS_EXC_RETURN_THREAD_PSP;
}

/**
 * @brief  Internal helper to validate task parameters and register task into task table.
 * @return RTOS_OK on success, or error code on invalid parameters or limit exceeded.
 */
static int rtos_task_add(RTOS_Task_t *task,
                         RTOS_TaskFunction_t function,
                         void *argument,
                         RTOS_StackWord_t *stack,
                         uint32_t stack_words,
                         uint8_t priority,
                         const char *name) {
  if ((task == (RTOS_Task_t *)0) ||
      (function == (RTOS_TaskFunction_t)0) ||
      (stack == (RTOS_StackWord_t *)0) ||
      (stack_words < RTOS_MIN_STACK_WORDS) ||
      ((((uintptr_t)stack) & 7U) != 0U) ||
      ((stack_words & 1U) != 0U) ||
      (priority >= RTOS_PRIORITY_LEVELS)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_task_count >= (RTOS_MAX_TASKS + 1U)) {
    return RTOS_ERR_LIMIT;
  }

  /* Verify stack space overlap with existing tasks */
  {
    uintptr_t new_low = (uintptr_t)stack;
    uintptr_t new_high = new_low +
                         ((uintptr_t)stack_words * sizeof(RTOS_StackWord_t));
    uint32_t existing_index;
    for (existing_index = 0U; existing_index < rtos_task_count;
         existing_index++) {
      RTOS_Task_t *existing = rtos_tasks[existing_index];
      uintptr_t existing_low = (uintptr_t)existing->stack_base;
      uintptr_t existing_high = existing_low +
          ((uintptr_t)existing->stack_words * sizeof(RTOS_StackWord_t));
      if ((existing == task) ||
          ((new_low < existing_high) && (existing_low < new_high))) {
        return RTOS_ERR_INVALID;
      }
    }
  }

  /* Initialize TCB fields */
  task->function = function;
  task->argument = argument;
  task->stack_base = stack;
  task->stack_words = stack_words;
  task->name = name;
  task->wait_object = (void *)0;
  task->wait_buffer = (void *)0;
  task->wake_tick = 0U;
  task->wait_sequence = 0U;
  task->wait_result = RTOS_OK;
  task->state = RTOS_TASK_READY;
  task->base_priority = priority;
  task->effective_priority = priority;
  task->wait_type = RTOS_WAIT_NONE;
  task->wait_forever = 0U;
  task->mutex_hold_count = 0U;

  rtos_stack_initialize(task);
  rtos_tasks[rtos_task_count++] = task;
  return RTOS_OK;
}

/**
 * @brief  Selects the next highest-priority ready task using round-robin per priority level.
 * @return Pointer to target task TCB, or NULL if no tasks ready.
 */
static RTOS_Task_t *rtos_select_next(void) {
  int32_t priority;
  uint32_t offset;
  uint32_t start;

  for (priority = (int32_t)RTOS_PRIORITY_LEVELS - 1; priority >= 0;
       priority--) {
    start = (uint32_t)rtos_rr_cursor[(uint32_t)priority];
    for (offset = 1U; offset <= rtos_task_count; offset++) {
      uint32_t index = (start + offset) % rtos_task_count;
      RTOS_Task_t *task = rtos_tasks[index];
      if (((task->state == RTOS_TASK_READY) ||
           (task->state == RTOS_TASK_RUNNING)) &&
          (task->effective_priority == (uint8_t)priority)) {
        rtos_rr_cursor[(uint32_t)priority] = (uint8_t)index;
        return task;
      }
    }
  }
  return (RTOS_Task_t *)0;
}

/**
 * @brief  Initializes the RTOS kernel state, NVIC exception priorities, and idle task.
 * @param  config Pointer to RTOS configuration structure.
 * @return RTOS_OK on success, or error code on invalid parameters or state.
 */
int rtos_init(const RTOS_Config_t *config) {
  uint32_t index;
  int rc;

  if ((config == (const RTOS_Config_t *)0) || (config->tick_hz == 0U) ||
      (config->max_syscall_irq_priority == 0U) ||
      (config->max_syscall_irq_priority >= 15U)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_running != 0U) {
    return RTOS_ERR_STATE;
  }

  rtos_task_count = 0U;
  rtos_application_task_count = 0U;
  rtos_wait_sequence = 0U;
  rtos_current_task = (RTOS_Task_t *)0;
  for (index = 0U; index < RTOS_PRIORITY_LEVELS; index++) {
    rtos_rr_cursor[index] = 0U;
  }
  rtos_fault_record.signature = 0U;
  rtos_fault_record.reason = RTOS_FAULT_NONE;
  rtos_config = *config;
  rtos_max_syscall_priority_mask =
      ((uint32_t)config->max_syscall_irq_priority << 4U);

#ifndef RTOS_HOST_TEST
  /* Configure NVIC priority grouping and system exception priorities */
  (void)nvic_set_priority_grouping(NVIC_PRIORITY_GROUP_4);
  (void)nvic_set_priority(NVIC_IRQ_SVCALL, 14U);
  (void)nvic_set_priority(NVIC_IRQ_PENDSV, 15U);
  (void)nvic_set_priority(NVIC_IRQ_SYSTICK, 15U);

  /* Enable FPU coprocessor access (CP10 & CP11) and automatic lazy stacking */
  SCB->CPACR |= (0xFUL << 20U);
  __asm volatile("dsb" ::: "memory");
  __asm volatile("isb" ::: "memory");
  FPU_FPCCR |= FPU_FPCCR_ASPEN | FPU_FPCCR_LSPEN;
#endif

  /* Create default background idle task at priority 0 */
  rc = rtos_task_add(&rtos_idle_task, rtos_idle_entry, (void *)0,
                     rtos_idle_stack, RTOS_IDLE_STACK_WORDS,
                     RTOS_IDLE_PRIORITY, "idle");
  if (rc != RTOS_OK) {
    return rc;
  }
  rtos_initialized = 1U;
  return RTOS_OK;
}

/**
 * @brief  Statically creates a new application task.
 * @param  task Pointer to task TCB storage.
 * @param  function Task entry function pointer.
 * @param  argument User argument pointer passed to task function.
 * @param  stack Pointer to static stack word buffer.
 * @param  stack_words Size of stack buffer in 32-bit words.
 * @param  priority Task priority level (1 to RTOS_PRIORITY_LEVELS - 1).
 * @param  name Descriptive task name string.
 * @return RTOS_OK on success, or error code on invalid parameters or limits.
 */
int rtos_task_create_static(RTOS_Task_t *task,
                            RTOS_TaskFunction_t function,
                            void *argument,
                            RTOS_StackWord_t *stack,
                            uint32_t stack_words,
                            uint8_t priority,
                            const char *name) {
  int rc;
  if ((rtos_initialized == 0U) || (rtos_running != 0U)) {
    return RTOS_ERR_STATE;
  }
  if ((priority == RTOS_IDLE_PRIORITY) ||
      (rtos_application_task_count >= RTOS_MAX_TASKS)) {
    return (priority == RTOS_IDLE_PRIORITY) ? RTOS_ERR_INVALID :
                                             RTOS_ERR_LIMIT;
  }
  rc = rtos_task_add(task, function, argument, stack, stack_words, priority,
                     name);
  if (rc == RTOS_OK) {
    rtos_application_task_count++;
  }
  return rc;
}

/**
 * @brief  Starts the RTOS preemptive task scheduler and launches the first task.
 * @return RTOS_ERR_STATE on failure (function does not return on success).
 */
int rtos_start(void) {
  SysTick_Config_t systick_config;
  RTOS_Task_t *first;
  int rc;

  if ((rtos_initialized == 0U) || (rtos_running != 0U) ||
      (rtos_application_task_count == 0U)) {
    return RTOS_ERR_STATE;
  }

  systick_config.clk_source = SYSTICK_CLKSRC_AHB;
  systick_config.tick_freq_hz = rtos_config.tick_hz;
  rc = systick_register_kernel_hook(rtos_tick_isr);
  if (rc != 0) {
    return RTOS_ERR_STATE;
  }
  rc = systick_configure_stopped(&systick_config);
  if (rc != 0) {
    return RTOS_ERR_INVALID;
  }

  first = rtos_select_next();
  if (first == (RTOS_Task_t *)0) {
    return RTOS_ERR_STATE;
  }
  first->state = RTOS_TASK_RUNNING;
  rtos_current_task = first;
  rtos_running = 1U;

  /* Execute SVC 0 instruction to switch to Process Stack Pointer (PSP) and start first task */
  rtos_port_start_first_task();
  return RTOS_ERR_STATE;
}

/**
 * @brief  Voluntarily yields remaining CPU execution time of current task.
 */
void rtos_task_yield(void) {
  if ((rtos_running != 0U) && (rtos_port_get_ipsr() == 0U)) {
    rtos_port_pend_context_switch();
  }
}

/**
 * @brief  Delays execution of the calling task for a given number of tick cycles.
 * @param  ticks Delay duration in SysTick units.
 * @return RTOS_OK on success, or error code on invalid parameters or state.
 */
int rtos_task_delay(uint32_t ticks) {
  RTOS_Task_t *self;
  uint32_t key;

  if (ticks == 0U) {
    rtos_task_yield();
    return RTOS_OK;
  }
  if ((rtos_kernel_thread_api_valid() == 0) ||
      (rtos_timeout_valid(ticks) == 0) ||
      (ticks == RTOS_WAIT_FOREVER)) {
    return RTOS_ERR_INVALID;
  }

  self = rtos_current_task;
  key = rtos_port_enter_critical();
  self->wake_tick = systick_get_tick() + ticks;
  self->wait_forever = 0U;
  self->state = RTOS_TASK_DELAYED;
  rtos_port_pend_context_switch();
  rtos_port_exit_critical(key);
  return RTOS_OK;
}

/**
 * @brief  Gets current system tick counter value.
 * @return Accumulated SysTick count.
 */
uint32_t rtos_tick_get(void) { return systick_get_tick(); }

/**
 * @brief  Internal kernel routine to transition current task to BLOCKED state.
 * @return RTOS_OK on success, or RTOS_ERR_INVALID on invalid parameters.
 */
int rtos_kernel_block_current(RTOS_WaitType_t wait_type,
                              void *object,
                              void *buffer,
                              uint32_t timeout_ticks) {
  RTOS_Task_t *task = rtos_current_task;
  if ((task == (RTOS_Task_t *)0) ||
      (rtos_timeout_valid(timeout_ticks) == 0)) {
    return RTOS_ERR_INVALID;
  }

  task->wait_type = (uint8_t)wait_type;
  task->wait_object = object;
  task->wait_buffer = buffer;
  task->wait_sequence = ++rtos_wait_sequence;
  task->wait_result = RTOS_OK;
  task->wait_forever = (timeout_ticks == RTOS_WAIT_FOREVER) ? 1U : 0U;
  if (task->wait_forever == 0U) {
    task->wake_tick = systick_get_tick() + timeout_ticks;
  }
  task->state = RTOS_TASK_BLOCKED;
  rtos_port_pend_context_switch();
  return RTOS_OK;
}

/**
 * @brief  Finds highest-priority (and oldest FIFO order) task waiting on object.
 * @return Pointer to best matching task TCB, or NULL if no waiter.
 */
RTOS_Task_t *rtos_kernel_find_waiter(RTOS_WaitType_t wait_type,
                                     const void *object) {
  RTOS_Task_t *best = (RTOS_Task_t *)0;
  uint32_t index;
  for (index = 0U; index < rtos_task_count; index++) {
    RTOS_Task_t *candidate = rtos_tasks[index];
    if ((candidate->state != RTOS_TASK_BLOCKED) ||
        (candidate->wait_type != (uint8_t)wait_type) ||
        (candidate->wait_object != object)) {
      continue;
    }
    if ((best == (RTOS_Task_t *)0) ||
        (candidate->effective_priority > best->effective_priority) ||
        ((candidate->effective_priority == best->effective_priority) &&
         ((int32_t)(candidate->wait_sequence - best->wait_sequence) < 0))) {
      best = candidate;
    }
  }
  return best;
}

/**
 * @brief  Unblocks specified task and sets its unblock result code.
 */
void rtos_kernel_wake_task(RTOS_Task_t *task, int result) {
  if (task == (RTOS_Task_t *)0) {
    return;
  }
  task->wait_result = result;
  task->wait_type = RTOS_WAIT_NONE;
  task->wait_object = (void *)0;
  task->wait_buffer = (void *)0;
  task->wait_forever = 0U;
  task->state = RTOS_TASK_READY;
}

/**
 * @brief  Recalculates effective priorities across all tasks for priority inheritance.
 */
void rtos_kernel_recompute_priorities(void) {
  uint32_t pass;
  uint32_t index;

  for (index = 0U; index < rtos_task_count; index++) {
    rtos_tasks[index]->effective_priority = rtos_tasks[index]->base_priority;
  }

  for (pass = 0U; pass < rtos_task_count; pass++) {
    uint8_t changed = 0U;
    for (index = 0U; index < rtos_task_count; index++) {
      RTOS_Task_t *waiter = rtos_tasks[index];
      if ((waiter->state == RTOS_TASK_BLOCKED) &&
          (waiter->wait_type == RTOS_WAIT_MUTEX)) {
        RTOS_Mutex_t *mutex = (RTOS_Mutex_t *)waiter->wait_object;
        if ((mutex != (RTOS_Mutex_t *)0) &&
            (mutex->owner != (RTOS_Task_t *)0) &&
            (mutex->owner->effective_priority < waiter->effective_priority)) {
          mutex->owner->effective_priority = waiter->effective_priority;
          changed = 1U;
        }
      }
    }
    if (changed == 0U) {
      break;
    }
  }
}

/**
 * @brief  Verifies if current context is valid for calling thread-level blocking APIs.
 * @return 1 if valid Thread mode context while kernel running, 0 otherwise.
 */
int rtos_kernel_thread_api_valid(void) {
  return ((rtos_running != 0U) && (rtos_port_get_ipsr() == 0U) &&
          (rtos_current_task != (RTOS_Task_t *)0)) ? 1 : 0;
}

/**
 * @brief  Checks if kernel has been initialized.
 * @return 1 if initialized, 0 otherwise.
 */
int rtos_kernel_is_initialized(void) {
  return (rtos_initialized != 0U) ? 1 : 0;
}

/**
 * @brief  Checks if kernel scheduler is running.
 * @return 1 if running, 0 otherwise.
 */
int rtos_kernel_is_running(void) { return (rtos_running != 0U) ? 1 : 0; }

/**
 * @brief  Checks if candidate task priority is strictly higher than current task priority.
 * @return 1 if candidate should preempt current task, 0 otherwise.
 */
int rtos_kernel_should_preempt(const RTOS_Task_t *task) {
  return ((task != (const RTOS_Task_t *)0) &&
          (rtos_current_task != (RTOS_Task_t *)0) &&
          (task->effective_priority >
           rtos_current_task->effective_priority)) ? 1 : 0;
}

/**
 * @brief  System tick handler called on SysTick interrupt. Checks delay/timeout expirations.
 */
void rtos_tick_isr(void) {
  uint32_t now;
  uint32_t index;
  uint8_t mutex_timeout = 0U;

  if (rtos_running == 0U) {
    return;
  }
  now = systick_get_tick();
  for (index = 0U; index < rtos_task_count; index++) {
    RTOS_Task_t *task = rtos_tasks[index];
    if ((task->state == RTOS_TASK_DELAYED) &&
        (rtos_deadline_reached(now, task->wake_tick) != 0)) {
      task->state = RTOS_TASK_READY;
    } else if ((task->state == RTOS_TASK_BLOCKED) &&
               (task->wait_forever == 0U) &&
               (rtos_deadline_reached(now, task->wake_tick) != 0)) {
      if (task->wait_type == RTOS_WAIT_MUTEX) {
        mutex_timeout = 1U;
      }
      rtos_kernel_wake_task(task, RTOS_ERR_TIMEOUT);
    }
  }
  if (mutex_timeout != 0U) {
    rtos_kernel_recompute_priorities();
  }
  rtos_port_pend_context_switch();
}

/**
 * @brief  Invoked by PendSV assembly handler to pick next task and validate stack bounds.
 */
void rtos_schedule_from_pendsv(void) {
  RTOS_Task_t *next;
  RTOS_Task_t *current = rtos_current_task;

  if ((current != (RTOS_Task_t *)0) &&
      (rtos_task_stack_valid(current) == 0)) {
    rtos_kernel_fault(RTOS_FAULT_STACK, current);
  }
  if ((current != (RTOS_Task_t *)0) &&
      (current->state == RTOS_TASK_RUNNING)) {
    current->state = RTOS_TASK_READY;
  }
  next = rtos_select_next();
  if (next == (RTOS_Task_t *)0) {
    rtos_kernel_fault(RTOS_FAULT_KERNEL_INVARIANT, current);
  }
  next->state = RTOS_TASK_RUNNING;
  rtos_current_task = next;
}

/**
 * @brief  Starts SysTick hardware timer during SVC handler startup.
 */
void rtos_start_tick_from_svc(void) { systick_start_internal(); }

/**
 * @brief  Trampoline wrapper for executing tasks; catches invalid task returns.
 * @param  task Pointer to target task TCB.
 */
void rtos_task_entry_trampoline(RTOS_Task_t *task) {
  task->function(task->argument);

  if (task->mutex_hold_count != 0U) {
    rtos_kernel_fault(RTOS_FAULT_TASK_RETURN_WITH_MUTEX, task);
  }
  {
    uint32_t key = rtos_port_enter_critical();
    task->state = RTOS_TASK_TERMINATED;
    rtos_port_pend_context_switch();
    rtos_port_exit_critical(key);
  }
  while (1) {
    rtos_port_wait_for_interrupt();
  }
}

/**
 * @brief  Weak default fault hook; can be overridden by user application.
 * @param  reason Reason for kernel fault.
 * @param  task Task associated with fault (or NULL).
 */
__attribute__((weak)) void rtos_fault_hook(RTOS_FaultReason_t reason,
                                           const RTOS_Task_t *task) {
  (void)reason;
  (void)task;
}

/**
 * @brief  Handles fatal kernel panics, logs fault details, and enters infinite loop.
 */
void rtos_kernel_fault(RTOS_FaultReason_t reason, const RTOS_Task_t *task) {
  rtos_fault_record.signature = RTOS_FAULT_SIGNATURE;
  rtos_fault_record.reason = reason;
  rtos_fault_record.task = task;
  rtos_fault_record.psp = rtos_port_get_psp();
  rtos_fault_record.msp = rtos_port_get_msp();
#ifndef RTOS_HOST_TEST
  rtos_fault_record.hfsr = SCB->HFSR;
  rtos_fault_record.cfsr = SCB->CFSR;
  rtos_fault_record.mmfar = SCB->MMFAR;
  rtos_fault_record.bfar = SCB->BFAR;
  __asm volatile("cpsid i" ::: "memory");
#endif
  rtos_fault_hook(reason, task);
  while (1) {
  }
}

/**
 * @brief  Captures stacked registers and system fault registers when HardFault occurs.
 * @param  frame Pointer to stacked CPU register frame.
 * @param  exc_return Value of EXC_RETURN in LR at exception entry.
 */
void rtos_hardfault_capture(uint32_t *frame, uint32_t exc_return) {
  rtos_fault_record.signature = RTOS_FAULT_SIGNATURE;
  rtos_fault_record.reason = RTOS_FAULT_HARDFAULT;
  rtos_fault_record.task = rtos_current_task;
  rtos_fault_record.psp = rtos_port_get_psp();
  rtos_fault_record.msp = rtos_port_get_msp();
  rtos_fault_record.exc_return = exc_return;
  if (frame != (uint32_t *)0) {
    rtos_fault_record.stacked_r0 = frame[0];
    rtos_fault_record.stacked_r1 = frame[1];
    rtos_fault_record.stacked_r2 = frame[2];
    rtos_fault_record.stacked_r3 = frame[3];
    rtos_fault_record.stacked_r12 = frame[4];
    rtos_fault_record.stacked_lr = frame[5];
    rtos_fault_record.stacked_pc = frame[6];
    rtos_fault_record.stacked_xpsr = frame[7];
  }
#ifndef RTOS_HOST_TEST
  rtos_fault_record.hfsr = SCB->HFSR;
  rtos_fault_record.cfsr = SCB->CFSR;
  rtos_fault_record.mmfar = SCB->MMFAR;
  rtos_fault_record.bfar = SCB->BFAR;
#endif
  rtos_fault_hook(RTOS_FAULT_HARDFAULT, rtos_current_task);
  while (1) {
  }
}

#ifdef RTOS_HOST_TEST
/** @brief Resets RTOS internal state for host unit tests. */
void rtos_test_reset(void) {
  rtos_running = 0U;
  rtos_initialized = 0U;
  rtos_current_task = (RTOS_Task_t *)0;
}
/** @brief Host unit test tick trigger wrapper. */
void rtos_test_tick(void) { rtos_tick_isr(); }
/** @brief Host unit test next task selector wrapper. */
RTOS_Task_t *rtos_test_select_next(void) { return rtos_select_next(); }
/** @brief Host unit test current running task override wrapper. */
void rtos_test_set_running(RTOS_Task_t *task) {
  if ((rtos_current_task != (RTOS_Task_t *)0) &&
      (rtos_current_task->state == RTOS_TASK_RUNNING)) {
    rtos_current_task->state = RTOS_TASK_READY;
  }
  rtos_running = 1U;
  rtos_current_task = task;
  if (task != (RTOS_Task_t *)0) {
    task->state = RTOS_TASK_RUNNING;
  }
}
#endif

