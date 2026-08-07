/**
 * @file    rtos_port.c
 * @brief   Cortex-M4 special-register helpers and architecture porting layer for mini-RTOS.
 */

#include "rtos_internal.h"
#include "stm32f411_xe.h"

/**
 * @brief  Enters a kernel critical section by masking interrupts at or below max_syscall_priority.
 * @details Reads the current BASEPRI register value, updates BASEPRI using basepri_max to block 
 *          kernel-aware interrupts, and inserts memory barriers to ensure instruction synchronization.
 * @return Previous BASEPRI value to be passed to rtos_port_exit_critical().
 */
uint32_t rtos_port_enter_critical(void) {
  uint32_t previous;
  __asm volatile("mrs %0, basepri" : "=r"(previous) :: "memory");
  __asm volatile("msr basepri_max, %0" ::
                     "r"(rtos_max_syscall_priority_mask) : "memory");
  __asm volatile("dsb" ::: "memory");
  __asm volatile("isb" ::: "memory");
  return previous;
}

/**
 * @brief  Exits a kernel critical section by restoring the BASEPRI register value.
 * @param  previous_basepri BASEPRI value saved prior to entering the critical section.
 */
void rtos_port_exit_critical(uint32_t previous_basepri) {
  __asm volatile("msr basepri, %0" :: "r"(previous_basepri) : "memory");
}

/**
 * @brief  Reads the Interrupt Program Status Register (IPSR).
 * @return Active exception vector number (0 = Thread mode, >0 = Handler mode / ISR).
 */
uint32_t rtos_port_get_ipsr(void) {
  uint32_t value;
  __asm volatile("mrs %0, ipsr" : "=r"(value));
  return value;
}

/**
 * @brief  Reads the Process Stack Pointer (PSP).
 * @return Current PSP address value.
 */
uint32_t rtos_port_get_psp(void) {
  uint32_t value;
  __asm volatile("mrs %0, psp" : "=r"(value));
  return value;
}

/**
 * @brief  Reads the Main Stack Pointer (MSP).
 * @return Current MSP address value.
 */
uint32_t rtos_port_get_msp(void) {
  uint32_t value;
  __asm volatile("mrs %0, msp" : "=r"(value));
  return value;
}

/**
 * @brief  Checks whether the active ISR vector is kernel-aware.
 * @details Checks if current execution is in an interrupt handler (vector >= 16) and 
 *          verifies if its priority is numerically >= max_syscall_priority (i.e. lower or equal priority in Cortex-M terms).
 * @return 1 if executed from a kernel-aware ISR, 0 otherwise.
 */
int rtos_port_is_kernel_aware_isr(void) {
  uint32_t exception = rtos_port_get_ipsr() & 0x1FFU;
  uint32_t irq;
  uint8_t priority;

  /* Return 0 if in Thread mode or system exception (NMI, HardFault, etc.) */
  if (exception < 16U) {
    return 0;
  }
  irq = exception - 16U;
  if (irq > 85U) {
    return 0;
  }
  /* Extract 4-bit NVIC priority from IP register */
  priority = (uint8_t)(NVIC->IP[irq] >> 4U);
  return ((priority >= (uint8_t)(rtos_max_syscall_priority_mask >> 4U)) &&
          (priority < 15U)) ? 1 : 0;
}

/**
 * @brief  Requests a context switch by setting the PendSV pending bit in ICSR.
 */
void rtos_port_pend_context_switch(void) {
  SCB->ICSR = SCB_ICSR_PENDSVSET;
  __asm volatile("dsb" ::: "memory");
  __asm volatile("isb" ::: "memory");
}

/**
 * @brief  Triggers Supervisor Call (SVC 0) to start the first task.
 */
void rtos_port_start_first_task(void) {
  __asm volatile("svc 0" ::: "memory");
}

/**
 * @brief  Puts the CPU into low-power Wait For Interrupt (WFI) mode during idle.
 */
void rtos_port_wait_for_interrupt(void) {
  __asm volatile("dsb" ::: "memory");
  __asm volatile("wfi");
  __asm volatile("isb" ::: "memory");
}

