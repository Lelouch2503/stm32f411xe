/**
 * @file    startup_stm32f411ve.s
 * @brief   STM32F411VETx startup file (GNU AS)
 *
 * This file contains:
 *  - Initial stack pointer value (_estack)
 *  - Cortex-M4 + STM32F411 full vector table
 *  - Reset_Handler: copies .data from Flash to RAM, zeroes .bss, calls main()
 *  - Default_Handler: infinite loop for unhandled interrupts
 *  - Weak aliases so each ISR can be overridden by user code
 */

    .syntax unified
    .cpu    cortex-m4
    .fpu    fpv4-sp-d16
    .thumb

/*----------------------------------------------------------------------
 * Linker symbols (defined in STM32F411VETx.ld)
 *--------------------------------------------------------------------*/
    .word   _sidata         /* Start address of .data initializers in Flash */
    .word   _sdata          /* Start address of .data in RAM               */
    .word   _edata          /* End   address of .data in RAM               */
    .word   _sbss           /* Start address of .bss  in RAM               */
    .word   _ebss           /* End   address of .bss  in RAM               */

/*======================================================================
 * Vector Table  –  placed at the very beginning of Flash (.isr_vector)
 *====================================================================*/
    .section .isr_vector, "a", %progbits
    .type   g_pfnVectors, %object
    .size   g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    /* Cortex-M4 system exceptions */
    .word   _estack                 /* 0x0000  Initial Stack Pointer           */
    .word   Reset_Handler           /* 0x0004  Reset                           */
    .word   NMI_Handler             /* 0x0008  Non-Maskable Interrupt          */
    .word   HardFault_Handler       /* 0x000C  Hard Fault                      */
    .word   MemManage_Handler       /* 0x0010  Memory Management Fault         */
    .word   BusFault_Handler        /* 0x0014  Bus Fault                       */
    .word   UsageFault_Handler      /* 0x0018  Usage Fault                     */
    .word   0                       /* 0x001C  Reserved                        */
    .word   0                       /* 0x0020  Reserved                        */
    .word   0                       /* 0x0024  Reserved                        */
    .word   0                       /* 0x0028  Reserved                        */
    .word   SVC_Handler             /* 0x002C  SVCall                          */
    .word   DebugMon_Handler        /* 0x0030  Debug Monitor                   */
    .word   0                       /* 0x0034  Reserved                        */
    .word   PendSV_Handler          /* 0x0038  PendSV                          */
    .word   SysTick_Handler         /* 0x003C  SysTick                         */

    /* STM32F411 peripheral interrupts (IRQ 0 – 85) */
    .word   WWDG_IRQHandler                 /* IRQ  0  */
    .word   PVD_IRQHandler                  /* IRQ  1  */
    .word   TAMP_STAMP_IRQHandler           /* IRQ  2  */
    .word   RTC_WKUP_IRQHandler             /* IRQ  3  */
    .word   FLASH_IRQHandler                /* IRQ  4  */
    .word   RCC_IRQHandler                  /* IRQ  5  */
    .word   EXTI0_IRQHandler                /* IRQ  6  */
    .word   EXTI1_IRQHandler                /* IRQ  7  */
    .word   EXTI2_IRQHandler                /* IRQ  8  */
    .word   EXTI3_IRQHandler                /* IRQ  9  */
    .word   EXTI4_IRQHandler                /* IRQ 10  */
    .word   DMA1_Stream0_IRQHandler         /* IRQ 11  */
    .word   DMA1_Stream1_IRQHandler         /* IRQ 12  */
    .word   DMA1_Stream2_IRQHandler         /* IRQ 13  */
    .word   DMA1_Stream3_IRQHandler         /* IRQ 14  */
    .word   DMA1_Stream4_IRQHandler         /* IRQ 15  */
    .word   DMA1_Stream5_IRQHandler         /* IRQ 16  */
    .word   DMA1_Stream6_IRQHandler         /* IRQ 17  */
    .word   ADC_IRQHandler                  /* IRQ 18  */
    .word   0                               /* IRQ 19  Reserved */
    .word   0                               /* IRQ 20  Reserved */
    .word   0                               /* IRQ 21  Reserved */
    .word   0                               /* IRQ 22  Reserved */
    .word   EXTI9_5_IRQHandler              /* IRQ 23  */
    .word   TIM1_BRK_TIM9_IRQHandler        /* IRQ 24  */
    .word   TIM1_UP_TIM10_IRQHandler        /* IRQ 25  */
    .word   TIM1_TRG_COM_TIM11_IRQHandler   /* IRQ 26  */
    .word   TIM1_CC_IRQHandler              /* IRQ 27  */
    .word   TIM2_IRQHandler                 /* IRQ 28  */
    .word   TIM3_IRQHandler                 /* IRQ 29  */
    .word   TIM4_IRQHandler                 /* IRQ 30  */
    .word   I2C1_EV_IRQHandler              /* IRQ 31  */
    .word   I2C1_ER_IRQHandler              /* IRQ 32  */
    .word   I2C2_EV_IRQHandler              /* IRQ 33  */
    .word   I2C2_ER_IRQHandler              /* IRQ 34  */
    .word   SPI1_IRQHandler                 /* IRQ 35  */
    .word   SPI2_IRQHandler                 /* IRQ 36  */
    .word   USART1_IRQHandler               /* IRQ 37  */
    .word   USART2_IRQHandler               /* IRQ 38  */
    .word   0                               /* IRQ 39  Reserved */
    .word   EXTI15_10_IRQHandler            /* IRQ 40  */
    .word   RTC_Alarm_IRQHandler            /* IRQ 41  */
    .word   OTG_FS_WKUP_IRQHandler          /* IRQ 42  */
    .word   0                               /* IRQ 43  Reserved */
    .word   0                               /* IRQ 44  Reserved */
    .word   0                               /* IRQ 45  Reserved */
    .word   0                               /* IRQ 46  Reserved */
    .word   DMA1_Stream7_IRQHandler         /* IRQ 47  */
    .word   0                               /* IRQ 48  Reserved */
    .word   SDIO_IRQHandler                 /* IRQ 49  */
    .word   TIM5_IRQHandler                 /* IRQ 50  */
    .word   SPI3_IRQHandler                 /* IRQ 51  */
    .word   0                               /* IRQ 52  Reserved */
    .word   0                               /* IRQ 53  Reserved */
    .word   0                               /* IRQ 54  Reserved */
    .word   0                               /* IRQ 55  Reserved */
    .word   DMA2_Stream0_IRQHandler         /* IRQ 56  */
    .word   DMA2_Stream1_IRQHandler         /* IRQ 57  */
    .word   DMA2_Stream2_IRQHandler         /* IRQ 58  */
    .word   DMA2_Stream3_IRQHandler         /* IRQ 59  */
    .word   DMA2_Stream4_IRQHandler         /* IRQ 60  */
    .word   0                               /* IRQ 61  Reserved */
    .word   0                               /* IRQ 62  Reserved */
    .word   0                               /* IRQ 63  Reserved */
    .word   0                               /* IRQ 64  Reserved */
    .word   0                               /* IRQ 65  Reserved */
    .word   0                               /* IRQ 66  Reserved */
    .word   OTG_FS_IRQHandler               /* IRQ 67  */
    .word   DMA2_Stream5_IRQHandler         /* IRQ 68  */
    .word   DMA2_Stream6_IRQHandler         /* IRQ 69  */
    .word   DMA2_Stream7_IRQHandler         /* IRQ 70  */
    .word   USART6_IRQHandler               /* IRQ 71  */
    .word   I2C3_EV_IRQHandler              /* IRQ 72  */
    .word   I2C3_ER_IRQHandler              /* IRQ 73  */
    .word   0                               /* IRQ 74  Reserved */
    .word   0                               /* IRQ 75  Reserved */
    .word   0                               /* IRQ 76  Reserved */
    .word   0                               /* IRQ 77  Reserved */
    .word   0                               /* IRQ 78  Reserved */
    .word   0                               /* IRQ 79  Reserved */
    .word   0                               /* IRQ 80  Reserved */
    .word   FPU_IRQHandler                  /* IRQ 81  */
    .word   0                               /* IRQ 82  Reserved */
    .word   0                               /* IRQ 83  Reserved */
    .word   SPI4_IRQHandler                 /* IRQ 84  */
    .word   SPI5_IRQHandler                 /* IRQ 85  */

/*======================================================================
 * Reset_Handler  –  first code that runs after power-on / reset
 *====================================================================*/
    .section .text.Reset_Handler
    .weak   Reset_Handler
    .type   Reset_Handler, %function

Reset_Handler:
    /* 1. Set stack pointer (already done by hardware from vector table,
          but some debuggers skip the vector table) */
    ldr     sp, =_estack

    /* 2. Copy .data section from Flash (LMA) to RAM (VMA) */
    ldr     r0, =_sdata         /* destination start */
    ldr     r1, =_edata         /* destination end   */
    ldr     r2, =_sidata        /* source start      */
    b       .L_copy_check
.L_copy_loop:
    ldr     r3, [r2], #4
    str     r3, [r0], #4
.L_copy_check:
    cmp     r0, r1
    blt     .L_copy_loop

    /* 3. Zero-fill .bss section */
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0
    b       .L_zero_check
.L_zero_loop:
    str     r2, [r0], #4
.L_zero_check:
    cmp     r0, r1
    blt     .L_zero_loop

    /* 4. Call the application entry point */
    bl      main

    /* 5. If main() ever returns, hang here */
.L_hang:
    b       .L_hang

    .size   Reset_Handler, . - Reset_Handler

/*======================================================================
 * Default_Handler  –  infinite loop for unimplemented ISRs
 *====================================================================*/
    .section .text.Default_Handler, "ax", %progbits
Default_Handler:
    b       Default_Handler
    .size   Default_Handler, . - Default_Handler

/*======================================================================
 * Weak aliases – each ISR defaults to Default_Handler unless overridden
 *====================================================================*/
    .weak   NMI_Handler
    .thumb_set NMI_Handler, Default_Handler

    .weak   HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler

    .weak   MemManage_Handler
    .thumb_set MemManage_Handler, Default_Handler

    .weak   BusFault_Handler
    .thumb_set BusFault_Handler, Default_Handler

    .weak   UsageFault_Handler
    .thumb_set UsageFault_Handler, Default_Handler

    .weak   SVC_Handler
    .thumb_set SVC_Handler, Default_Handler

    .weak   DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler

    .weak   PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler

    .weak   SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

    .weak   WWDG_IRQHandler
    .thumb_set WWDG_IRQHandler, Default_Handler

    .weak   PVD_IRQHandler
    .thumb_set PVD_IRQHandler, Default_Handler

    .weak   TAMP_STAMP_IRQHandler
    .thumb_set TAMP_STAMP_IRQHandler, Default_Handler

    .weak   RTC_WKUP_IRQHandler
    .thumb_set RTC_WKUP_IRQHandler, Default_Handler

    .weak   FLASH_IRQHandler
    .thumb_set FLASH_IRQHandler, Default_Handler

    .weak   RCC_IRQHandler
    .thumb_set RCC_IRQHandler, Default_Handler

    .weak   EXTI0_IRQHandler
    .thumb_set EXTI0_IRQHandler, Default_Handler

    .weak   EXTI1_IRQHandler
    .thumb_set EXTI1_IRQHandler, Default_Handler

    .weak   EXTI2_IRQHandler
    .thumb_set EXTI2_IRQHandler, Default_Handler

    .weak   EXTI3_IRQHandler
    .thumb_set EXTI3_IRQHandler, Default_Handler

    .weak   EXTI4_IRQHandler
    .thumb_set EXTI4_IRQHandler, Default_Handler

    .weak   DMA1_Stream0_IRQHandler
    .thumb_set DMA1_Stream0_IRQHandler, Default_Handler

    .weak   DMA1_Stream1_IRQHandler
    .thumb_set DMA1_Stream1_IRQHandler, Default_Handler

    .weak   DMA1_Stream2_IRQHandler
    .thumb_set DMA1_Stream2_IRQHandler, Default_Handler

    .weak   DMA1_Stream3_IRQHandler
    .thumb_set DMA1_Stream3_IRQHandler, Default_Handler

    .weak   DMA1_Stream4_IRQHandler
    .thumb_set DMA1_Stream4_IRQHandler, Default_Handler

    .weak   DMA1_Stream5_IRQHandler
    .thumb_set DMA1_Stream5_IRQHandler, Default_Handler

    .weak   DMA1_Stream6_IRQHandler
    .thumb_set DMA1_Stream6_IRQHandler, Default_Handler

    .weak   ADC_IRQHandler
    .thumb_set ADC_IRQHandler, Default_Handler

    .weak   EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler, Default_Handler

    .weak   TIM1_BRK_TIM9_IRQHandler
    .thumb_set TIM1_BRK_TIM9_IRQHandler, Default_Handler

    .weak   TIM1_UP_TIM10_IRQHandler
    .thumb_set TIM1_UP_TIM10_IRQHandler, Default_Handler

    .weak   TIM1_TRG_COM_TIM11_IRQHandler
    .thumb_set TIM1_TRG_COM_TIM11_IRQHandler, Default_Handler

    .weak   TIM1_CC_IRQHandler
    .thumb_set TIM1_CC_IRQHandler, Default_Handler

    .weak   TIM2_IRQHandler
    .thumb_set TIM2_IRQHandler, Default_Handler

    .weak   TIM3_IRQHandler
    .thumb_set TIM3_IRQHandler, Default_Handler

    .weak   TIM4_IRQHandler
    .thumb_set TIM4_IRQHandler, Default_Handler

    .weak   I2C1_EV_IRQHandler
    .thumb_set I2C1_EV_IRQHandler, Default_Handler

    .weak   I2C1_ER_IRQHandler
    .thumb_set I2C1_ER_IRQHandler, Default_Handler

    .weak   I2C2_EV_IRQHandler
    .thumb_set I2C2_EV_IRQHandler, Default_Handler

    .weak   I2C2_ER_IRQHandler
    .thumb_set I2C2_ER_IRQHandler, Default_Handler

    .weak   SPI1_IRQHandler
    .thumb_set SPI1_IRQHandler, Default_Handler

    .weak   SPI2_IRQHandler
    .thumb_set SPI2_IRQHandler, Default_Handler

    .weak   USART1_IRQHandler
    .thumb_set USART1_IRQHandler, Default_Handler

    .weak   USART2_IRQHandler
    .thumb_set USART2_IRQHandler, Default_Handler

    .weak   EXTI15_10_IRQHandler
    .thumb_set EXTI15_10_IRQHandler, Default_Handler

    .weak   RTC_Alarm_IRQHandler
    .thumb_set RTC_Alarm_IRQHandler, Default_Handler

    .weak   OTG_FS_WKUP_IRQHandler
    .thumb_set OTG_FS_WKUP_IRQHandler, Default_Handler

    .weak   DMA1_Stream7_IRQHandler
    .thumb_set DMA1_Stream7_IRQHandler, Default_Handler

    .weak   SDIO_IRQHandler
    .thumb_set SDIO_IRQHandler, Default_Handler

    .weak   TIM5_IRQHandler
    .thumb_set TIM5_IRQHandler, Default_Handler

    .weak   SPI3_IRQHandler
    .thumb_set SPI3_IRQHandler, Default_Handler

    .weak   DMA2_Stream0_IRQHandler
    .thumb_set DMA2_Stream0_IRQHandler, Default_Handler

    .weak   DMA2_Stream1_IRQHandler
    .thumb_set DMA2_Stream1_IRQHandler, Default_Handler

    .weak   DMA2_Stream2_IRQHandler
    .thumb_set DMA2_Stream2_IRQHandler, Default_Handler

    .weak   DMA2_Stream3_IRQHandler
    .thumb_set DMA2_Stream3_IRQHandler, Default_Handler

    .weak   DMA2_Stream4_IRQHandler
    .thumb_set DMA2_Stream4_IRQHandler, Default_Handler

    .weak   OTG_FS_IRQHandler
    .thumb_set OTG_FS_IRQHandler, Default_Handler

    .weak   DMA2_Stream5_IRQHandler
    .thumb_set DMA2_Stream5_IRQHandler, Default_Handler

    .weak   DMA2_Stream6_IRQHandler
    .thumb_set DMA2_Stream6_IRQHandler, Default_Handler

    .weak   DMA2_Stream7_IRQHandler
    .thumb_set DMA2_Stream7_IRQHandler, Default_Handler

    .weak   USART6_IRQHandler
    .thumb_set USART6_IRQHandler, Default_Handler

    .weak   I2C3_EV_IRQHandler
    .thumb_set I2C3_EV_IRQHandler, Default_Handler

    .weak   I2C3_ER_IRQHandler
    .thumb_set I2C3_ER_IRQHandler, Default_Handler

    .weak   FPU_IRQHandler
    .thumb_set FPU_IRQHandler, Default_Handler

    .weak   SPI4_IRQHandler
    .thumb_set SPI4_IRQHandler, Default_Handler

    .weak   SPI5_IRQHandler
    .thumb_set SPI5_IRQHandler, Default_Handler

    .end
