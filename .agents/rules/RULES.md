---
trigger: always_on
---

# STM32F411 Bare-Metal Development Rules


## 2. Source-of-Truth Rules

1. Treat the STM32F411 reference manual, datasheet, board schematic, and existing project register definitions as the primary technical sources.
2. Do not guess:
   - Register bit positions
   - Alternate-function numbers
   - IRQ numbers
   - DMA stream/channel mappings
   - Clock limits
   - Flash wait-state requirements
   - Peripheral reset behavior
3. When information is uncertain, explicitly identify the assumption and request the relevant reference-manual section, schematic, or existing source file where necessary.
4. Preserve consistency with the existing project architecture before introducing a new abstraction or naming convention.

---

## 3. Bare-Metal Architecture Rules

1. Maintain the **No HAL, No CMSIS** design philosophy.
2. Peripheral driver public interfaces must be placed in:

   ```text
   inc/stm32f411_<peripheral>.h
   ```

3. Peripheral driver implementations must be placed in:

   ```text
   src/stm32f411_<peripheral>.c
   ```

4. Hardware register definitions belong in:

   ```text
   inc/stm32f411_xe.h
   ```

5. Startup, vector-table, memory-initialization, and reset-level logic belongs in:

   ```text
   startup/startup_stm32f411ve.s
   ```

6. Do not put complex driver logic directly in `main.c`. `main.c` should primarily initialize the system, configure drivers, and run application logic.
7. Prefer small, focused peripheral drivers over a monolithic board-support source file.
8. Do not bypass a public driver API from application code unless direct register access is explicitly required for startup, fault handling, or a documented low-level exception.

---

## 4. Register Access Rules

1. Use the project’s peripheral base-pointer macros, such as:

   ```c
   #define RCC ((RCC_TypeDef *)RCC_BASE)
   ```

2. Use volatile-qualified register access through `__IO`, `__I`, and `__O`.
3. Do not cache a memory-mapped peripheral register in a non-volatile local variable when polling for hardware state changes.

   Incorrect:

   ```c
   uint32_t sr = USART1->SR.reg;
   while (!(sr & USART_SR_TXE)) {
   }
   ```

   Correct:

   ```c
   while (!USART1->SR.bit.TXE) {
   }
   ```

4. Re-read volatile status registers in every polling-loop iteration.
5. Use read-modify-write operations carefully. Never unintentionally overwrite reserved bits, write-one-to-clear flags, or bits owned by hardware.
6. Follow the register-specific clearing sequence defined by the STM32 reference manual. For example, some status flags require reading a status register followed by a data register read.
7. Perform dummy reads after enabling a peripheral clock when required for bus synchronization:

   ```c
   rcc_apb1_clk_enable(...);
   (void)RCC->APB1ENR;
   ```

8. Use memory barriers where required by architectural or peripheral sequencing:

   ```c
   __asm volatile("dsb" ::: "memory");
   __asm volatile("isb" ::: "memory");
   ```

9. Configure GPIO alternate-function registers and GPIO mode before enabling peripheral operation on that pin.

---

## 5. Clock and Reset Rules

1. Never exceed STM32F411 clock-domain limits:
   - SYSCLK: maximum 100 MHz
   - APB1: maximum 50 MHz
   - APB2: maximum 100 MHz
2. Configure Flash latency **before** switching to a higher clock frequency.
3. For the standard 100 MHz configuration at 3.3 V, use 3 Flash wait states.
4. When reconfiguring the PLL:
   1. Switch SYSCLK away from PLL, typically to HSI.
   2. Disable PLL.
   3. Wait until PLL is ready to be reconfigured.
   4. Update PLL parameters.
   5. Enable PLL and wait for lock.
   6. Switch SYSCLK to PLL.
5. Never assume HSE is available without checking its ready flag and using a timeout.
6. Every oscillator, PLL, reset, and clock-switch polling loop must have a finite timeout.
7. Return an error when the clock source does not become ready before timeout; do not continue with an invalid clock configuration.
8. Enable the FPU before any floating-point operation because the build uses hard-float ABI and `fpv4-sp-d16`.

   ```c
   SCB->CPACR |= (0xF << 20);
   __asm volatile("dsb" ::: "memory");
   __asm volatile("isb" ::: "memory");
   ```

---

## 6. GPIO and Peripheral Configuration Rules

1. Enable the corresponding RCC peripheral clock before configuring a peripheral or GPIO port.
2. Configure pins explicitly:
   - Mode
   - Output type
   - Output speed
   - Pull-up/pull-down
   - Alternate function, where relevant
3. Do not leave critical communication pins in an unspecified GPIO state.
4. Validate that the selected pin supports the requested peripheral alternate function.
5. Before enabling a peripheral:
   - Configure its clock source and prescaler.
   - Configure GPIO pins.
   - Clear stale status flags when applicable.
   - Configure interrupts and NVIC if interrupt-driven mode is used.
   - Configure DMA if DMA mode is used.
6. Do not enable a peripheral interrupt until its state, buffers, callbacks/state machine, and status flags are in a safe initial state.
7. For polling-based APIs, always provide a timeout or documented non-blocking behavior.
8. Avoid hidden blocking delays in reusable driver APIs unless the API name and documentation clearly indicate blocking behavior.

---

## 7. Interrupt and NVIC Rules

1. IRQ handlers must use the exact symbol names defined by the startup vector table:

   ```c
   void USART1_IRQHandler(void);
   void TIM2_IRQHandler(void);
   ```

2. Do not rename handlers or create wrapper handlers unless the vector table is explicitly updated.
3. Clear or consume the interrupt source before returning from an ISR.
4. Keep ISRs short, deterministic, and non-blocking.
5. Do not call long delays, polling loops, clock reconfiguration routines, or blocking communication APIs from an ISR.
6. Do not use dynamic memory allocation inside an ISR.
7. Avoid `printf`, semihosting, and lengthy logging inside an ISR.
8. Share data between ISR and foreground code safely:
   - Mark shared state as `volatile` where appropriate.
   - Use atomic access patterns for single-word data where valid.
   - Protect multi-step shared-state modifications when necessary.
9. Use the NVIC directly through its registers; do not rely on CMSIS intrinsics.
10. Keep the default priority grouping unless a driver has a documented reason to change it.
11. Assign interrupt priorities intentionally. Time-critical interrupts should not be blocked by lower-value work.

---

## 8. Driver API Rules

1. Every public driver function must have a Doxygen comment.
2. Use the project naming conventions:
   - Types: `PascalCase_t`
   - Enums/constants: `UPPER_SNAKE_CASE`
   - Functions: `lower_snake_case()`
   - Prefix all driver symbols with the peripheral or module name.
3. Use configuration structures for non-trivial peripheral initialization.
4. Validate all public API parameters.
5. Return `int` from fallible driver APIs:
   - `0`: success
   - `-1`: invalid parameter
   - `-2`: timeout
   - `-3`: unsupported configuration
   - `-4`: peripheral busy or locked
6. Document any driver-specific error code in the public header.
7. The caller must check return values from configuration, transmit, receive, and clock APIs.
8. Do not silently ignore hardware timeouts, invalid settings, or peripheral errors.
9. Prefer explicit state transitions over undocumented implicit behavior.
10. Do not expose internal helper functions in public headers unless they are intentionally part of the supported API.

---

## 9. Memory, Startup, and Fault-Safety Rules

1. Preserve the startup sequence:
   - Initialize stack pointer.
   - Copy `.data` from Flash to RAM.
   - Zero `.bss`.
   - Enable FPU access.
   - Call `main()`.
2. `main()` must not return. If it does, enter a safe infinite loop.
3. Do not use heap allocation unless a heap implementation, memory limits, fragmentation policy, and failure handling are explicitly defined.
4. Avoid recursion in production embedded code unless stack usage is known and justified.
5. Consider stack usage whenever adding nested calls, large local arrays, interrupt nesting, or floating-point code.
6. Implement or preserve a useful `HardFault_Handler` that allows fault-register inspection:
   - `SCB->HFSR`
   - `SCB->CFSR`
   - `SCB->MMFAR`
   - `SCB->BFAR`
7. Never mask a HardFault by resetting blindly without preserving diagnostic evidence during development.

---

## 10. Build and Toolchain Rules

1. Use the configured `arm-none-eabi-gcc` cross-compiler.
2. Preserve target flags unless there is an explicit reason to change them:

   ```text
   -mcpu=cortex-m4
   -mthumb
   -mfloat-abi=hard
   -mfpu=fpv4-sp-d16
   ```

3. Use C99-compatible source code.
4. Preserve linker garbage collection support:

   ```text
   -ffunction-sections
   -fdata-sections
   -Wl,--gc-sections
   ```

5. Do not add startup files from the standard C runtime when the project intentionally uses a custom startup file and `-nostartfiles`.
6. Build at least Debug configuration before flashing new functionality.
7. Check build warnings. Do not dismiss warnings that may indicate:
   - Incorrect pointer casts
   - Integer truncation
   - Signed/unsigned bugs
   - Uninitialized variables
   - Missing prototypes
   - Incorrect format assumptions
8. Check binary size after substantial changes, especially Flash and RAM consumption.

---

## 11. Debugging and Validation Rules

1. Validate hardware-dependent functionality on real hardware; compilation alone is insufficient.
2. After clock configuration changes, verify basic execution using an LED heartbeat or GDB.
3. For UART, SPI, I2C, PWM, and timing-sensitive signals, validate with appropriate equipment when available:
   - Logic analyzer
   - Oscilloscope
   - Protocol analyzer
4. Prefer GDB register inspection, watchpoints, and memory examination over adding uncontrolled `printf` debugging.
5. Use a RAM circular buffer with SysTick timestamps when non-intrusive event logging is required.
6. Investigate the root cause of a fault before applying a workaround.
7. If OpenOCD cannot halt the target because firmware is trapped in a fault loop, use reset-assisted attachment.
8. Treat every timeout, bus error, overrun, framing error, arbitration loss, or unexpected status flag as an observable diagnostic condition.

---
## 12. Code Style and Documentation Rules

1. Use C99.
2. Use 2-space indentation.
3. Put opening braces on the same line as functions and control statements.
4. Use include guards in all public headers.
5. Preserve C++ compatibility wrappers in public headers:

   ```c
   #ifdef __cplusplus
   extern "C" {
   #endif
   ```

6. Use descriptive names; avoid unexplained magic values.
7. Define register masks, bit positions, timing constants, and protocol limits as named macros or enums.
8. Add step-by-step comments for complex sequences such as:
   - Clock setup
   - UART initialization
   - I2C transactions
   - DMA setup
   - Interrupt initialization
9. Do not write comments that contradict the code.
10. Update documentation whenever a public API, error code, hardware assumption, or configuration constraint changes.

---

## 13. Response Rules for the Firmware Agent

When generating firmware changes, the agent must:

1. Identify the target peripheral, pins, clock domain, interrupt/DMA dependencies, and expected behavior.
2. State assumptions explicitly when board wiring or external hardware is not specified.
3. Provide complete changes across all required files, not isolated code fragments only.
4. Include initialization order and error handling.
5. Avoid claiming that code has been flashed, compiled, or hardware-tested unless execution evidence is provided.
6. Clearly distinguish:
   - Code that is directly derived from known project conventions
   - Assumptions requiring validation against the STM32F411 reference manual or board schematic
7. Include a concise verification procedure after implementing a feature.
```
