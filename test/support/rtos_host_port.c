#include "rtos_internal.h"

static uint32_t host_tick;
static uint32_t host_ipsr;
static uint8_t host_kernel_aware_isr;
static void (*host_tick_hook)(void);

uint32_t rtos_port_enter_critical(void) { return 0U; }
void rtos_port_exit_critical(uint32_t previous_basepri) {
  (void)previous_basepri;
}
uint32_t rtos_port_get_ipsr(void) { return host_ipsr; }
uint32_t rtos_port_get_psp(void) { return 0U; }
uint32_t rtos_port_get_msp(void) { return 0U; }
int rtos_port_is_kernel_aware_isr(void) {
  return (host_kernel_aware_isr != 0U) ? 1 : 0;
}
void rtos_port_pend_context_switch(void) {}
void rtos_port_start_first_task(void) {}
void rtos_port_wait_for_interrupt(void) {}

uint32_t systick_get_tick(void) { return host_tick; }
int systick_configure_stopped(const SysTick_Config_t *cfg) {
  return ((cfg != (const SysTick_Config_t *)0) &&
          (cfg->tick_freq_hz != 0U)) ? 0 : -1;
}
void systick_start_internal(void) {}
int systick_register_kernel_hook(void (*hook)(void)) {
  host_tick_hook = hook;
  return (hook != (void (*)(void))0) ? 0 : -1;
}

void rtos_test_set_tick(uint32_t tick) { host_tick = tick; }

void rtos_host_set_isr(uint32_t ipsr, int kernel_aware) {
  host_ipsr = ipsr;
  host_kernel_aware_isr = (kernel_aware != 0) ? 1U : 0U;
}

void rtos_host_fire_tick(void) {
  host_tick++;
  if (host_tick_hook != (void (*)(void))0) {
    host_tick_hook();
  }
}
