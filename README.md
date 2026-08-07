# STM32F411E-DISCO bare-metal mini-RTOS

![CI](https://github.com/Lelouch2503/stm32f411xe/actions/workflows/ci.yml/badge.svg)

This repository contains register-level STM32F411VETx drivers and a small,
statically allocated preemptive RTOS. It uses no STM32 HAL, CMSIS device
headers, or third-party RTOS kernel.

## Kernel model

- Cortex-M4F privileged tasks run on PSP; exceptions run on MSP.
- SVC starts the first task, PendSV switches context, and SysTick supplies a
  1 kHz default tick.
- Fixed priorities 1-7 are available to applications. Priority 0 belongs to
  the internal idle task. Equal-priority tasks use round-robin scheduling.
- A maximum of eight application tasks may be created before `rtos_start()`.
- Task control blocks, stacks, semaphores, mutexes, queues, and queue buffers
  are owned by the application and statically allocated.
- FPU lazy stacking is enabled. PendSV conditionally preserves S16-S31 when
  the interrupted task owns an extended floating-point exception frame.
- Kernel-aware interrupts must use priorities 5-14 by default. Priorities 0-4
  remain unmasked by RTOS critical sections and must not call `*_from_isr()`.

The public API is declared in `inc/rtos.h`; compile-time limits are in
`inc/rtos_config.h`. A zero IPC timeout performs a non-blocking poll and
`RTOS_WAIT_FOREVER` waits indefinitely.

## Board demonstration

The default application configures the HSE/PLL clock at 100 MHz and USART2 at
115200 baud. Green and blue LED tasks run at equal priority, a software-pended
TAMP_STAMP interrupt gives a semaphore to a higher-priority red LED task, and
a queue plus priority-inheritance mutex serialize UART logging. The orange LED
indicates initialization or kernel faults.

USART2 uses PA2 (TX) and PA3 (RX). Connect a 3.3 V USB-to-UART adapter with a
common ground.

## Build and test

```sh
make build
make release
cmake -B build-test -S test -DCMAKE_BUILD_TYPE=Debug
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

Use `make flash` for the on-board ST-Link/OpenOCD setup described by the
project Makefile. Hardware validation is still required for context-switch
timing, interrupt behavior, and FPU preservation.
