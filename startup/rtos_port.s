/**
 * @file    rtos_port.s
 * @brief   Cortex-M4F SVC, PendSV, and HardFault assembly exception handlers.
 */

    .syntax unified
    .cpu    cortex-m4
    .fpu    fpv4-sp-d16
    .thumb

    .equ TCB_SP_OFFSET,         0   /* Offset of stack_pointer field in Task Control Block */
    .equ TCB_EXC_RETURN_OFFSET, 4   /* Offset of exc_return field in Task Control Block */

/*======================================================================
 * SVC_Handler – Starts the first RTOS task
 *====================================================================*/
    .section .text.SVC_Handler, "ax", %progbits
    .global SVC_Handler
    .type   SVC_Handler, %function
    .thumb_func
SVC_Handler:
    /* Preserve r3 and lr on main stack for C function call */
    push    {r3, lr}
    bl      rtos_start_tick_from_svc
    pop     {r3, lr}

    /* main() will never resume: reclaim its call frames for exception MSP stack */
    ldr     r0, =_estack
    msr     msp, r0

    /* Load rtos_current_task address into r3, TCB pointer into r2 */
    ldr     r3, =rtos_current_task
    ldr     r2, [r3]

    /* Read initial stack pointer from task TCB (stack_pointer field) */
    ldr     r0, [r2, #TCB_SP_OFFSET]

    /* Pop software-saved registers (r4-r11) from initial task stack */
    ldmia   r0!, {r4-r11}

    /* Set PSP to point to initial hardware auto-stacked frame */
    msr     psp, r0

    /* Load initial EXC_RETURN value from task TCB into LR */
    ldr     lr, [r2, #TCB_EXC_RETURN_OFFSET]

    /* Switch CONTROL register bit 1 (use PSP, unprivileged/privileged Thread mode) */
    movs    r0, #2
    msr     control, r0
    isb

    /* Enable interrupts (clear PRIMASK) and branch to initial task entry via EXC_RETURN */
    cpsie   i
    bx      lr
    .size   SVC_Handler, . - SVC_Handler

/*======================================================================
 * PendSV_Handler – Asynchronous task context switch handler
 *====================================================================*/
    .section .text.PendSV_Handler, "ax", %progbits
    .global PendSV_Handler
    .type   PendSV_Handler, %function
    .thumb_func
PendSV_Handler:
    /* Read active Process Stack Pointer (PSP) */
    mrs     r0, psp
    isb
    ldr     r3, =rtos_current_task
    ldr     r2, [r3]

    /* Hardware saves S0-S15 when EXC_RETURN bit 4 is clear.
       Save remaining FPU registers (S16-S31) if bit 4 of LR is 0. */
    tst     lr, #0x10
    it      eq
    vstmdbeq r0!, {s16-s31}

    /* Push software context (R4-R11) onto current task stack */
    stmdb   r0!, {r4-r11}

    /* Save updated PSP stack pointer and EXC_RETURN code to current task TCB */
    str     r0, [r2, #TCB_SP_OFFSET]
    str     lr, [r2, #TCB_EXC_RETURN_OFFSET]

    /* Mask kernel-aware interrupts during scheduler task selection */
    ldr     r1, =rtos_max_syscall_priority_mask
    ldr     r1, [r1]
    msr     basepri, r1
    dsb
    isb

    /* Call C scheduler routine to pick next task */
    push    {r3, lr}
    bl      rtos_schedule_from_pendsv
    pop     {r3, lr}

    /* Unmask interrupts by resetting BASEPRI to 0 */
    movs    r1, #0
    msr     basepri, r1

    /* Load updated rtos_current_task (new task) pointer */
    ldr     r2, [r3]

    /* Restore new task stack pointer and EXC_RETURN code from its TCB */
    ldr     r0, [r2, #TCB_SP_OFFSET]
    ldr     lr, [r2, #TCB_EXC_RETURN_OFFSET]

    /* Pop software context (R4-R11) from new task stack */
    ldmia   r0!, {r4-r11}

    /* Restore FPU registers (S16-S31) if bit 4 of LR is 0 */
    tst     lr, #0x10
    it      eq
    vldmiaeq r0!, {s16-s31}

    /* Update PSP with new task stack pointer address */
    msr     psp, r0
    isb

    /* Return from PendSV exception to launch/resume target task */
    bx      lr
    .size   PendSV_Handler, . - PendSV_Handler

/*======================================================================
 * HardFault_Handler – Low-level HardFault entry handler
 *====================================================================*/
    .section .text.HardFault_Handler, "ax", %progbits
    .global HardFault_Handler
    .type   HardFault_Handler, %function
    .thumb_func
HardFault_Handler:
    /* Check EXC_RETURN bit 2 (0 = MSP used, 1 = PSP used) to select active stack pointer */
    tst     lr, #4
    ite     eq
    mrseq   r0, msp
    mrsne   r0, psp

    /* Pass EXC_RETURN value in R1 */
    mov     r1, lr

    /* If bit 4 is 0, FPU registers (S0-S15 + FPSCR) were stacked; adjust frame offset (+72 bytes) */
    tst     lr, #0x10
    it      eq
    addeq   r0, r0, #72

    /* Jump to C HardFault diagnosis capture routine (r0 = stack frame ptr, r1 = exc_return) */
    b       rtos_hardfault_capture
    .size   HardFault_Handler, . - HardFault_Handler

