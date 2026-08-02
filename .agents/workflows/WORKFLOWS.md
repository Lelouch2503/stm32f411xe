---
description: 
---

```md
---
name: STM32F411 Bare-Metal Development Workflow
description: >
  Standard workflows for planning, implementing, building, flashing, debugging,
  and validating bare-metal firmware features on the STM32F411E-DISCO.
---

# STM32F411 Bare-Metal Development Workflow

## 1. General Development Workflow

Use this workflow for every firmware request, whether it is a new driver, a bug fix,
a board-level feature, or an application change.

### Step 1 — Understand the Request

Identify:

1. Requested peripheral or feature
2. Expected input and output behavior
3. Whether the operation is polling, interrupt-driven, DMA-driven, or mixed
4. Required pins and external connections
5. Timing, throughput, power, and reliability requirements
6. Whether the feature must coexist with existing peripherals
7. Whether a clock-tree change is required

If any critical information is unavailable, state the assumption or ask for clarification.

Example questions:

- Which STM32F411E-DISCO pins are connected to the external module?
- Which UART/SPI/I2C instance is required?
- What baud rate, SPI mode, I2C speed, or timer period is needed?
- Must the API be blocking, non-blocking, interrupt-driven, or DMA-driven?
- Is the external device powered at 3.3 V and electrically compatible?

---

### Step 2 — Check Hardware Feasibility

Before coding, verify:

1. The requested peripheral exists on STM32F411VETx.
2. The requested pins support the peripheral alternate function.
3. The selected peripheral clock domain can satisfy timing requirements.
4. No pin conflicts exist with:
   - ST-Link/debug interfaces
   - User LEDs
   - User button
   - Existing peripheral assignments
5. The required IRQ and DMA resources are available, if applicable.
6. The requested timing is compatible with APB/AHB clock limits.

Output of this step:

```text
Peripheral:
GPIO pins:
Alternate function:
RCC clock domain:
IRQ:
DMA stream/channel, if used:
Clock/timing assumptions:
Potential conflicts:
```

---

### Step 3 — Design the Driver API

Create or update:

```text
inc/stm32f411_<peripheral>.h
```

Define:

1. Public enums
2. Configuration structures
3. Error codes
4. Public function prototypes
5. Doxygen comments
6. Optional state or callback types for asynchronous drivers

Example API design:

```c
typedef struct {
  uint32_t baudrate;
  USART_WordLength_t word_length;
  USART_Parity_t parity;
  USART_StopBits_t stop_bits;
} USART_Config_t;

/**
 * @brief Initialize a USART peripheral.
 *
 * @param instance USART register base pointer.
 * @param config User-provided USART configuration.
 * @return 0 on success, negative error code on failure.
 */
int usart_init(USART_TypeDef *instance, const USART_Config_t *config);
```

API design checklist:

- Is every parameter validated?
- Are the return values documented?
- Does the interface reveal required blocking behavior?
- Are hardware-specific details hidden where reasonable?
- Is the API consistent with existing RCC and SysTick driver conventions?

---

### Step 4 — Add or Verify Register Definitions

Inspect:

```text
inc/stm32f411_xe.h
```

Verify that it contains:

1. Peripheral base address
2. Register structure
3. Register offsets in the correct order
4. Bit masks or bit-field definitions
5. RCC enable/reset bits
6. IRQ-related definitions if needed

If register definitions are missing:

1. Add them using the project’s existing `union { reg; bit; }` pattern.
2. Preserve reserved fields and correct offsets.
3. Avoid adding unsupported registers from a different STM32 family.

---

### Step 5 — Implement the Driver

Create or update:

```text
src/stm32f411_<peripheral>.c
```

Implementation sequence:

1. Add internal helper functions as `static`.
2. Validate input parameters.
3. Enable the required RCC clock.
4. Perform a dummy RCC register read if required for synchronization.
5. Configure GPIO before enabling peripheral operation.
6. Reset or disable the peripheral before changing protected configuration fields if required.
7. Configure clock source, prescaler, mode, frame format, timing, and control registers.
8. Clear stale flags.
9. Configure NVIC and interrupt-enable bits for interrupt-driven operation.
10. Enable the peripheral.
11. Return `0` on success or a documented negative error code.

For polling operations:

1. Wait for the required status flag.
2. Decrement a timeout counter.
3. Return `-2` on timeout.
4. Handle peripheral error flags before returning.

---

### Step 6 — Integrate into `main.c`

In `main.c`:

1. Initialize system clock using the RCC driver.
2. Initialize SysTick if timing or timeout services require it.
3. Initialize GPIO/peripheral drivers.
4. Check every return code.
5. Enter the application main loop.
6. Keep application behavior separate from reusable driver implementation.

Example:

```c
int main(void) {
  int rc;

  rc = rcc_sys_clk_config(&clk_config);
  if (rc != 0) {
    while (1) {
      /* Clock configuration failure indication */
    }
  }

  rc = systick_init(&systick_config);
  if (rc != 0) {
    while (1) {
      /* SysTick initialization failure indication */
    }
  }

  rc = usart_init(USART2, &usart_config);
  if (rc != 0) {
    while (1) {
      /* USART initialization failure indication */
    }
  }

  while (1) {
    /* Application logic */
  }
}
```

---

### Step 7 — Build and Review

Build using:

```bash
make build
```

or:

```bash
make
```

Review:

1. Compiler errors
2. Compiler warnings
3. Linker errors
4. ELF section sizes
5. Generated `.bin` and `.hex` files

For size inspection:

```bash
make size
```

For disassembly inspection:

```bash
make disasm
```

Do not flash code that has unresolved warnings related to registers, pointer types,
interrupt handlers, uninitialized configuration, or memory layout.

---

### Step 8 — Flash Firmware

Flash with:

```bash
make flash
```

Expected tool chain:

```text
CMake/Make → arm-none-eabi-gcc → ELF/BIN/HEX → OpenOCD → ST-Link V2 → STM32F411
```

If OpenOCD cannot connect because the target is trapped in a fault loop:

1. Hold the board RESET button.
2. Run `make flash`.
3. Release RESET after OpenOCD establishes connection.

---

### Step 9 — Validate on Hardware

Validation must match the feature.

| Feature | Minimum Validation |
|---|---|
| Clock configuration | LED heartbeat, GDB clock-register inspection |
| GPIO output | LED or oscilloscope measurement |
| GPIO input/EXTI | Button press and interrupt verification |
| UART | Terminal output and loopback test |
| SPI | Logic analyzer capture; verify mode and data |
| I2C | Logic analyzer; ACK/NACK and bus recovery checks |
| Timer/PWM | Oscilloscope frequency and duty-cycle measurement |
| ADC | Compare readings against known voltage |
| DMA | Verify buffer contents, transfer count, completion IRQ |
| Interrupt driver | Confirm IRQ firing, flag clearing, priority behavior |

Record:

1. Test setup
2. Firmware configuration
3. Expected result
4. Observed result
5. Failure symptoms, if any

---

## 2. New Peripheral Driver Workflow

Use this workflow when adding a driver such as GPIO, USART, SPI, I2C, TIM, ADC, DMA,
EXTI, RTC, or watchdog.

### Phase A — Research

1. Identify the peripheral chapter in the STM32F411 reference manual.
2. Identify:
   - RCC enable/reset bits
   - Register sequence
   - GPIO AF mapping
   - Status flags
   - Error flags
   - Interrupt behavior
   - DMA support
   - Required timing equations
3. Identify any errata or board-level constraints.

### Phase B — Header

1. Add `inc/stm32f411_<peripheral>.h`.
2. Add include guards.
3. Add C++ linkage guards.
4. Define config enums and structures.
5. Define function prototypes and error semantics.
6. Add Doxygen documentation.

### Phase C — Register Layer

1. Add missing registers to `stm32f411_xe.h`.
2. Verify register offsets.
3. Add base addresses and RCC clock controls.
4. Add bit masks without affecting reserved bits.

### Phase D — Driver Layer

Implement in this order:

1. Peripheral clock enable
2. Peripheral reset, if appropriate
3. GPIO configuration support or documented GPIO prerequisite
4. Core initialization
5. Blocking transmit/receive or basic operation
6. Timeout/error handling
7. Interrupt support
8. DMA support, if needed
9. Deinitialization/reset support, if needed

### Phase E — Bring-Up Test

Create the smallest possible test:

- GPIO: blink an LED
- USART: send a fixed banner
- SPI: loopback transfer
- I2C: scan one known device address
- Timer: toggle a pin at a known rate
- ADC: sample a fixed/known voltage

Only expand the driver after the minimal test passes.

---

## 3. Clock Configuration Workflow

Use this workflow whenever changing SYSCLK, PLL, bus prescalers, or Flash latency.

### Step 1 — Define Target Frequencies

Specify:

```text
HSE or HSI source:
PLLM:
PLLN:
PLLP:
PLLQ:
SYSCLK:
AHB prescaler:
APB1 prescaler:
APB2 prescaler:
Flash latency:
```

For the standard 100 MHz HSE-based configuration:

```text
HSE = 8 MHz
PLLM = 4
PLLN = 200
PLLP = 4
PLLQ = 8

VCO = (8 / 4) × 200 = 400 MHz
SYSCLK = 400 / 4 = 100 MHz
APB1 = 100 / 2 = 50 MHz
APB2 = 100 / 1 = 100 MHz
Flash latency = 3 wait states
```

### Step 2 — Validate Limits

Confirm:

1. PLL input and VCO values are valid.
2. SYSCLK does not exceed 100 MHz.
3. APB1 does not exceed 50 MHz.
4. APB2 does not exceed 100 MHz.
5. Flash latency is sufficient before increasing clock speed.
6. Peripheral timing calculations are updated after the clock change.

### Step 3 — Apply Safe Transition

1. Enable HSI if it is not already running.
2. Switch SYSCLK to HSI.
3. Disable PLL.
4. Wait until PLL is disabled.
5. Configure prescalers and PLL settings.
6. Configure Flash latency.
7. Enable HSE if required.
8. Wait for HSE ready with timeout.
9. Enable PLL.
10. Wait for PLL ready with timeout.
11. Switch SYSCLK to PLL.
12. Verify that PLL is reported as the active system clock source.

### Step 4 — Validate

1. Blink an LED using SysTick or a timer.
2. Measure output frequency using an oscilloscope if possible.
3. Verify `rcc_get_*_freq()` or equivalent frequency readback.
4. Confirm UART baud rate and peripheral timing remain correct.

---

## 4. Interrupt-Driven Feature Workflow

Use this workflow for EXTI, USART RX/TX, timers, DMA completion, ADC end-of-conversion,
and similar interrupt-based functions.

### Step 1 — Define the Event Model

Specify:

1. Interrupt source
2. Trigger condition
3. Data shared with foreground code
4. Buffering mechanism
5. Error conditions
6. Required latency and priority
7. Foreground processing model

### Step 2 — Configure Hardware

1. Enable peripheral clock.
2. Configure GPIO/AF.
3. Clear pending peripheral flags.
4. Configure peripheral interrupt-enable bits.
5. Configure NVIC priority.
6. Enable NVIC IRQ.
7. Enable the peripheral.

### Step 3 — Implement ISR

ISR requirements:

1. Check source flags.
2. Read or clear flags in the correct order.
3. Store minimal necessary data.
4. Update `volatile` state or ring buffers.
5. Handle errors.
6. Return promptly.

Example skeleton:

```c
void USART1_IRQHandler(void) {
  if (USART1->SR.bit.RXNE) {
    uint8_t data = (uint8_t)USART1->DR.reg;
    /* Store data into a ring buffer. */
  }

  if (USART1->SR.bit.ORE) {
    (void)USART1->SR.reg;
    (void)USART1->DR.reg;
    /* Record overrun error. */
  }
}
```

### Step 4 — Validate

1. Force or generate the interrupt source.
2. Confirm the handler is entered with GDB breakpoint.
3. Confirm flags are cleared.
4. Confirm no interrupt storm occurs.
5. Confirm data integrity under repeated events.
6. Confirm foreground code safely consumes shared data.

---

## 5. Fault-Debugging Workflow

Use this workflow for HardFault, lockups, unexpected resets, or code that stops running.

### Step 1 — Preserve Fault Context

Implement a `HardFault_Handler` that captures:

```text
MSP/PSP
SCB->HFSR
SCB->CFSR
SCB->MMFAR
SCB->BFAR
```

Avoid immediately resetting the MCU during development.

### Step 2 — Attach GDB

Start OpenOCD:

```bash
make openocd
```

Start debugging:

```bash
make debug
```

Useful GDB commands:

```gdb
info registers
backtrace
x/16wx $sp
p/x SCB->HFSR
p/x SCB->CFSR
p/x SCB->MMFAR
p/x SCB->BFAR
```

### Step 3 — Check Common Root Causes

1. FPU used before CPACR enables CP10/CP11 access
2. Invalid peripheral base address
3. Peripheral clock not enabled
4. Incorrect register offset or bit mask
5. Null pointer access
6. Stack overflow
7. Invalid vector-table entry
8. ISR symbol does not match vector-table symbol
9. Unaligned access
10. Incorrect Flash latency during clock increase

### Step 4 — Fix and Revalidate

1. Apply the smallest correct fix.
2. Rebuild.
3. Flash.
4. Repeat the original failing scenario.
5. Confirm the fault registers no longer report the original error.

---

## 6. WSL/OpenOCD Troubleshooting Workflow

Use this workflow if `make flash` fails when invoking Windows OpenOCD from WSL.

### Symptom

```text
Exec format error
```

### Steps

1. Check WSL interop status:

   ```bash
   cat /proc/sys/fs/binfmt_misc/WSLInterop
   ```

2. Restart WSL from PowerShell:

   ```powershell
   wsl --shutdown
   ```

3. Reopen WSL and retry:

   ```bash
   make flash
   ```

4. Verify that `/etc/wsl.conf` does not disable interop.

5. If OpenOCD starts but cannot halt the target:
   - Use reset-assisted connection.
   - Check whether firmware is looping in `HardFault_Handler`.

---

## 7. Completion Checklist

Before considering a firmware task complete, verify:

### Design
- [ ] Peripheral and pin mapping are valid.
- [ ] Clock-domain limits are respected.
- [ ] Public API follows existing conventions.
- [ ] Error handling is defined.

### Implementation
- [ ] RCC clock enable is present.
- [ ] GPIO mode and AF are configured correctly.
- [ ] Polling loops have timeouts.
- [ ] Interrupt flags are cleared correctly.
- [ ] ISR is short and non-blocking.
- [ ] No HAL or CMSIS device headers were introduced.
- [ ] Register access is volatile-safe.

### Build
- [ ] Debug build succeeds.
- [ ] Warnings were reviewed and resolved.
- [ ] Flash/RAM size was checked where relevant.

### Hardware Validation
- [ ] Firmware was flashed successfully.
- [ ] Functional behavior was validated on hardware.
- [ ] Timing/protocol signals were measured where relevant.
- [ ] Failure conditions and timeout paths were tested.
- [ ] GDB/OpenOCD debugging was used for unresolved issues.

### Documentation
- [ ] Header API comments are complete.
- [ ] New assumptions are documented.
- [ ] Test procedure and expected behavior are documented.
```

---

## Suggested repository placement

```text
stm32f411xe/
├── SKILL.md
├── RULES.md
├── WORKFLOW.md
├── main.c
├── inc/
├── src/
├── startup/
├── cmake/
└── ...
```