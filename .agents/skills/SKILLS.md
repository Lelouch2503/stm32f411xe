---
name: STM32F411 Bare-Metal Development
description: >
  Skill for developing bare-metal firmware on the STM32F411xE (STM32F411E-DISCO board).
  Covers the project architecture, register-level driver patterns, build system (CMake + Make),
  toolchain (arm-none-eabi-gcc via WSL), flashing (OpenOCD), debugging (GDB), coding conventions,
  and peripheral driver API design. No HAL, no CMSIS — pure bare-metal.
---

# STM32F411 Bare-Metal Firmware Development

## 1. Project Overview

This project is a **bare-metal firmware** for the **STM32F411VETx** microcontroller
on the **STM32F411E-DISCO** discovery board. It deliberately avoids ST's HAL library
and the CMSIS device headers, implementing everything from scratch for full control
and educational clarity.

**Target MCU:** STM32F411VETx (Cortex-M4F, 100 MHz max, 512 KB Flash, 128 KB RAM)
**Board:** STM32F411E-DISCO (4 user LEDs on PD12–PD15, ST-Link V2 on-board)

## 2. Directory Structure

```
stm32f411xe/
├── main.c                          # Application entry point
├── inc/                            # Public header files
│   ├── stm32f411_xe.h              # Device-level register definitions (memory map,
│   │                               #   peripheral structs, bit-field macros)
│   ├── stm32f411_rcc.h             # RCC driver public API
│   └── stm32f411_systick.h         # SysTick driver public API
|   └── ...
├── src/                            # Driver implementation files
│   ├── stm32f411_rcc.c             # RCC driver (clock config, freq readback, clock gating)
│   └── stm32f411_systick.c         # SysTick driver (timebase, delay, timeout)
|   └── ...
├── startup/
│   └── startup_stm32f411ve.s       # Startup assembly (vector table, Reset_Handler,
│                                   #   .data copy, .bss zero, calls main)
├── STM32F411VETx.ld                # Linker script (512K Flash @ 0x08000000,
│                                   #   128K RAM @ 0x20000000)
├── CMakeLists.txt                  # CMake build configuration
├── cmake/
│   └── arm-none-eabi-gcc.cmake     # CMake cross-compilation toolchain file
|   └── ...
├── Makefile                        # Convenience wrapper (make build/flash/clean/debug…)
├── .vscode/
│   └── launch.json                 # VS Code debug configuration
|   └── ...
└── build/                          # Build output (generated)
```

## 3. Architecture & Design Philosophy

### 3.1 No HAL, No CMSIS
- All peripheral registers are defined manually in `inc/stm32f411_xe.h`
- Register structures use `union { __IO uint32_t reg; struct { ... } bit; }` pattern
  for both raw register access (`.reg`) and named bit-field access (`.bit`)
- Reference manual: **RM0383 Rev 3** (STM32F411xC/E)

### 3.2 Driver Pattern
Each peripheral driver follows this pattern:

```
inc/stm32f411_<peripheral>.h    → Public API (enums, config structs, function prototypes)
src/stm32f411_<peripheral>.c    → Implementation (static helpers + public functions)
```

**Naming convention:**
- Types: `RCC_ClkInit_t`, `SysTick_Config_t`,... (PascalCase with `_t` suffix)
- Enums: `RCC_SYSCLK_PLL`, `SYSTICK_CLKSRC_AHB`,... (UPPER_SNAKE_CASE, prefixed)
- Functions: `rcc_sys_clk_config()`, `systick_init()`,... (lower_snake_case, prefixed)
- Macros/Constants: `RCC_CR_HSEON`, `FLASH_ACR_LATENCY_Msk`,... (UPPER_SNAKE_CASE)

### 3.3 Configuration Structs
Drivers use configuration structs that the user fills in and passes by pointer:

Below is example for the implementation:
```c
RCC_ClkInit_t clk = {
    .sysclk_src     = RCC_SYSCLK_PLL,
    .ahb_prescaler  = RCC_AHB_DIV1,
    .apb1_prescaler = RCC_APB_DIV2,
    .apb2_prescaler = RCC_APB_DIV1,
    .flash_latency  = RCC_FLASH_LATENCY_3WS,
    .pll = {
        .PLL_Source = RCC_PLLSRC_HSE,
        .PLLM = 4, .PLLN = 200,
        .PLLP = RCC_PLLP_DIV4, .PLLQ = 8,
    },
};
int rc = rcc_sys_clk_config(&clk);
```

### 3.4 Error Handling
- Functions return `int`: `0` = success, negative = error code
- Standard error codes:
  | Code | Meaning                        |
  |------|-------------------------------|
  |  0   | Success                        |
  | -1   | Invalid parameter              |
  | -2   | Timeout (hardware not ready)   |
  | -3   | Unsupported configuration      |
  | -4   | Peripheral busy / locked       |
- Each driver documents its specific error codes in the header
- Caller should always check return value: `if (rc != 0) { /* handle */ }`
- No errno, no exceptions, no printf debugging

### 3.5 Register Access Conventions
- Peripheral base pointers are cast macros: `#define RCC ((RCC_TypeDef *)RCC_BASE)`
- Volatile access via `__IO` / `__I` / `__O` qualifiers
- Dummy reads after clock enable: `(void)RCC->APB1ENR;` for bus sync (ST errata)
- Memory barriers via `__asm volatile("dsb" ::: "memory")` where needed (e.g., reset)
- Timeout loops with decrementing counter to prevent infinite hangs, avoid using the delay API, which hangs the MCU for nothing.

### 3.6 Interrupt Handling & NVIC Convention
- Vector table is defined in `startup/startup_stm32f411ve.s` with weak symbol aliases
  for every IRQ handler, pointing to a default `Default_Handler` (infinite loop)
- To handle an interrupt, simply define a function with the exact matching symbol
  name in your driver's `.c` file — the linker overrides the weak alias automatically
- **Naming convention:** `<PERIPHERAL>_IRQHandler` (must match vector table exactly,
  e.g. `void USART1_IRQHandler(void)`, `void TIM2_IRQHandler(void)`)

**NVIC configuration — pure register access (no CMSIS intrinsics):**
| Register      | Purpose                                      |
|----------------|-----------------------------------------------|
| `NVIC->ISER[n]` | Enable interrupt (n = IRQ number / 32)       |
| `NVIC->ICER[n]` | Disable interrupt                           |
| `NVIC->ISPR[n]` | Set pending (force trigger, for testing)     |
| `NVIC->ICPR[n]` | Clear pending                               |
| `NVIC->IP[irq]` | Set priority (only upper 4 bits used, 0=highest) |

**Default priority grouping:** PRIGROUP = 0 → all 4 priority bits are
pre-emption priority (no sub-priority). Do not change unless a driver
explicitly requires priority nesting control.

**ISR pattern — always clear the flag before returning:**
```c
void USART1_IRQHandler(void) {
  if (USART1->SR.bit.RXNE) {
    uint8_t data = (uint8_t)USART1->DR.reg;  // read DR clears RXNE
    // handle received byte
  }
  if (USART1->SR.bit.ORE) {
    (void)USART1->SR.reg;  // dummy read clears overrun error
    (void)USART1->DR.reg;
  }
}

### 3.7 Startup Sequence & FPU Initialization
`Reset_Handler` in `startup_stm32f411ve.s` performs, in order:
1. Set `SP = _estack` (top of RAM, defined in `STM32F411VETx.ld`)
2. Copy `.data` section from Flash (LMA) to RAM (VMA)
3. Zero-fill `.bss` section
4. **Enable FPU access** (see below) — must happen before `main()` since any
   float/double variable or FPU register touch will fault otherwise
5. Call `main()`
6. If `main()` ever returns → infinite loop (should never happen in bare-metal)

**FPU enable — required because build uses `-mfloat-abi=hard -mfpu=fpv4-sp-d16`:**
```c
// Must run before ANY floating-point operation
SCB->CPACR |= (0xF << 20);  // Full access to CP10 & CP11 (FPU coprocessors)
__asm volatile("dsb" ::: "memory");
__asm volatile("isb" ::: "memory");

## 5. Build System

### 5.1 Toolchain
- **Compiler:** `arm-none-eabi-gcc` 10.3.1 (running in WSL)
- **CPU flags:** `-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16`
- **Specs:** `--specs=nosys.specs --specs=nano.specs -nostartfiles`
- **Sections:** `-ffunction-sections -fdata-sections -Wl,--gc-sections`

### 5.2 CMake
- Toolchain file: `cmake/arm-none-eabi-gcc.cmake`
- Build types: Debug (`-Og -g3 -gdwarf-2`) / Release (`-Os`)
- Sources are globbed from `src/*.c` plus explicit `main.c` and startup `.s`
- Post-build: generates `.bin`, `.hex`, prints `size`

### 5.3 Make Targets
| Target | Description |
|--------|-------------|
| `make` / `make build` | Configure + Build (Debug) |
| `make release` | Configure + Build (Release, -Os) |
| `make flash` | Build + Flash via OpenOCD |
| `make clean` | Remove build directory |
| `make rebuild` | Clean + Build |
| `make size` | Print .elf section sizes |
| `make disasm` | Disassemble .elf |
| `make openocd` | Start OpenOCD server |
| `make debug` | GDB debug session (needs openocd running) |

### 5.4 Flashing & Debugging
- **OpenOCD** (Windows executable called from WSL via interop):
  - Path: `/mnt/c/openocd/xpack-openocd-0.12.0-7/bin/openocd.exe`
  - Config: `interface/stlink.cfg` + `target/stm32f4x.cfg`
  - Requires WSL interop enabled (can break — see Troubleshooting)
- **GDB:** `gdb-multiarch` connecting to OpenOCD on `:3333`

## 6. Coding Conventions

### 6.1 Style
- C99 standard
- 2-space indentation
- `{` on same line for functions and control structures
- Doxygen `/** */` comments on all public API functions and struct fields
- Section headers with `/* ══════════ */` box-drawing characters
- Step-by-step numbered comments for complex sequences (e.g., clock config, uart config, i2c config,...)

### 6.2 Header Guards
```c
#ifndef STM32F411_<MODULE>_H
#define STM32F411_<MODULE>_H

#ifdef __cplusplus
extern "C" {
#endif

// ... contents ...

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_<MODULE>_H */
```

### 6.3 New Driver Checklist
When adding a new peripheral driver:
1. Create `inc/stm32f411_<periph>.h` with enums, config structs, and function prototypes
2. Create `src/stm32f411_<periph>.c` with implementation
3. Add necessary register structures to `inc/stm32f411_xe.h` if not already present
4. Source files in `src/` are auto-discovered by CMake (`file(GLOB SRC_FILES src/*.c)`)
5. Follow the existing `union { reg; struct { bit; } }` pattern for new register structs
6. Use `rcc_<bus>_clk_enable()` to enable peripheral clocks before configuration
7. Include timeout protection on all polling loops

### 6.4 Volatile & Optimization Pitfalls
- **Never** cache a peripheral register value into a non-volatile local variable
  inside a polling/timeout loop — the compiler may hoist the read out of the loop
  entirely under `-Og`/`-Os`, causing an infinite loop or stale-value bug:
  ```c
  // WRONG — compiler may read SR only once and cache it
  uint32_t sr = USART1->SR.reg;
  while (!(sr & USART_SR_TXE)) { }

  // CORRECT — re-read the volatile register struct member each iteration
  while (!USART1->SR.bit.TXE) { }

## 7. Hardware Notes (STM32F411E-DISCO)

| Feature | Details |
|---------|---------|
| HSE Crystal | 8 MHz (on-board) |
| HSI | 16 MHz (internal RC) |
| Max SYSCLK | 100 MHz |
| Flash | 512 KB @ 0x08000000 |
| RAM | 128 KB @ 0x20000000 |
| User LEDs | LD4 (Green) PD12, LD3 (Orange) PD13, LD5 (Red) PD14, LD6 (Blue) PD15 |
| User Button | PA0 (directly connected, active-high) |
| Debugger | ST-Link V2 (on-board) |
| APB1 max | 50 MHz |
| APB2 max | 100 MHz |

### 7.1 Alternate Function (AF) Pin Mapping — Common Peripherals
Always configure `GPIOx->AFR[0/1]` (AFRL for pins 0–7, AFRH for pins 8–15)
**before** enabling the peripheral's clock-dependent functionality, and set
`MODER` to Alternate Function mode (`0b10`) for the pin.

## 8. Testing & Validation Strategy
- No unit test framework → use hardware-in-the-loop validation
- LED blink as "heartbeat" sanity check after clock config
- Logic analyzer / oscilloscope for UART, SPI, I2C,... timing verification
- GDB watchpoints for register value inspection without printf
- SysTick timestamp logging via circular buffer in RAM (inspect via GDB)

## 9. Troubleshooting

### WSL Interop (Exec format error)
If `make flash` fails with `Exec format error` when calling `openocd.exe`:
1. WSL interop is disabled — check: `cat /proc/sys/fs/binfmt_misc/WSLInterop`
2. Fix: restart WSL from PowerShell: `wsl --shutdown`, then reopen
3. If persistent: `sudo sh -c 'echo :WSLInterop:M::MZ::/init:PF > /proc/sys/fs/binfmt_misc/register'`
4. Verify `/etc/wsl.conf` does not contain `[interop] enabled = false`

### Flash Latency
- Must set flash latency BEFORE switching to a faster clock
- At 3.3V: 0 WS ≤ 30 MHz, 1 WS ≤ 64 MHz, 2 WS ≤ 90 MHz, 3 WS ≤ 100 MHz

### PLL Reconfiguration
- Must switch to HSI and disable PLL before changing PLL parameters
- The driver handles this automatically in `rcc_sys_clk_config()`

### Hard Fault Debugging
Common causes in bare-metal:
1. Unaligned memory access with strict alignment enabled
2. FPU instruction before CPACR enable → UsageFault escalated to HardFault
3. Stack overflow (default stack size in linker script too small)
4. Null pointer dereference (peripheral base macro wrong)
5. Missing clock enable before peripheral register access

Debug approach:
- Implement HardFault_Handler that captures MSP/PSP and fault registers:
  SCB->HFSR, SCB->CFSR, SCB->MMFAR, SCB->BFAR
- Use GDB: `info registers` + `backtrace` at fault

### OpenOCD "Error: Target not halted"
- Cause: MCU in HardFault loop, ST-Link can't attach cleanly
- Fix: Hold RESET button while running `make flash`, release after OpenOCD connects

### Clock Configuration Fails (rcc_sys_clk_config returns -2)
- Check HSE crystal is populated (some boards ship without it)
- Verify BYPASS bit if using external oscillator vs. crystal
- Check flash latency is set BEFORE clock switch [1]

## 10. Clock Tree Summary
HSI (16MHz) ──┐
              ├──> PLL ──> SYSCLK ──> AHB ──> APB1 (÷2, max 50MHz)
HSE (8MHz)  ──┘                            └──> APB2 (÷1, max 100MHz)

Recommended 100MHz config (from HSE):
  PLLM=4, PLLN=200, PLLP=DIV4, PLLQ=8
  → VCO = 8/4 * 200 = 400 MHz
  → SYSCLK = 400/4 = 100 MHz ✓
  → USB/SDIO = 400/8 = 50 MHz ✓ (not 48MHz exactly — USB not supported at 100MHz config)
  Flash latency: 3WS required [1]
