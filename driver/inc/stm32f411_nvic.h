/**
 * @file    stm32f411_nvic.h
 * @brief   NVIC driver for STM32F411xE — public API
 *
 * Provides functions to:
 *  - Enable and disable peripheral interrupts
 *  - Set and clear interrupt pending state
 *  - Configure interrupt preemption and sub-priority grouping
 *  - Set and get interrupt priority for both system exceptions and peripheral IRQs
 *
 * Reference: ARMv7-M Architecture Reference Manual §B3.4
 */

#ifndef STM32F411_NVIC_H
#define STM32F411_NVIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f411_xe.h"

/* ══════════════════════════════════════════════════════════════════════
 * Enumerations
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief STM32F411xE Interrupt Request Numbers.
 *
 * Negative values represent system exceptions (Cortex-M4 core-specific).
 * Non-negative values represent STM32F411xE peripheral interrupts.
 */
typedef enum {
  /* ── System Exceptions (Cortex-M4 Core) ────────────────────────── */
  NVIC_IRQ_MEMMANAGE           = -12, /**< Memory Management Fault        */
  NVIC_IRQ_BUSFAULT            = -11, /**< Bus Fault                      */
  NVIC_IRQ_USAGEFAULT          = -10, /**< Usage Fault                    */
  NVIC_IRQ_SVCALL              = -5,  /**< System Service Call            */
  NVIC_IRQ_PENDSV              = -2,  /**< Pendable System Service        */
  NVIC_IRQ_SYSTICK             = -1,  /**< System Tick Timer              */

  /* ── Peripheral Interrupts (STM32F411xE) ───────────────────────── */
  NVIC_IRQ_WWDG                = 0,   /**< Window Watchdog Interrupt      */
  NVIC_IRQ_PVD                 = 1,   /**< PVD through EXTI Line detect   */
  NVIC_IRQ_TAMP_STAMP          = 2,   /**< Tamper and TimeStamp           */
  NVIC_IRQ_RTC_WKUP            = 3,   /**< RTC Wakeup through EXTI line   */
  NVIC_IRQ_FLASH               = 4,   /**< Flash global Interrupt         */
  NVIC_IRQ_RCC                 = 5,   /**< RCC global Interrupt           */
  NVIC_IRQ_EXTI0               = 6,   /**< EXTI Line0 Interrupt           */
  NVIC_IRQ_EXTI1               = 7,   /**< EXTI Line1 Interrupt           */
  NVIC_IRQ_EXTI2               = 8,   /**< EXTI Line2 Interrupt           */
  NVIC_IRQ_EXTI3               = 9,   /**< EXTI Line3 Interrupt           */
  NVIC_IRQ_EXTI4               = 10,  /**< EXTI Line4 Interrupt           */
  NVIC_IRQ_DMA1_STREAM0        = 11,  /**< DMA1 Stream0 global Interrupt  */
  NVIC_IRQ_DMA1_STREAM1        = 12,  /**< DMA1 Stream1 global Interrupt  */
  NVIC_IRQ_DMA1_STREAM2        = 13,  /**< DMA1 Stream2 global Interrupt  */
  NVIC_IRQ_DMA1_STREAM3        = 14,  /**< DMA1 Stream3 global Interrupt  */
  NVIC_IRQ_DMA1_STREAM4        = 15,  /**< DMA1 Stream4 global Interrupt  */
  NVIC_IRQ_DMA1_STREAM5        = 16,  /**< DMA1 Stream5 global Interrupt  */
  NVIC_IRQ_DMA1_STREAM6        = 17,  /**< DMA1 Stream6 global Interrupt  */
  NVIC_IRQ_ADC                 = 18,  /**< ADC1 global Interrupt          */
  NVIC_IRQ_EXTI9_5             = 23,  /**< EXTI Line[9:5] Interrupts      */
  NVIC_IRQ_TIM1_BRK_TIM9       = 24,  /**< TIM1 Break / TIM9 global       */
  NVIC_IRQ_TIM1_UP_TIM10       = 25,  /**< TIM1 Update / TIM10 global     */
  NVIC_IRQ_TIM1_TRG_COM_TIM11  = 26,  /**< TIM1 Trigger/Commut. / TIM11   */
  NVIC_IRQ_TIM1_CC             = 27,  /**< TIM1 Capture Compare Interrupt */
  NVIC_IRQ_TIM2                = 28,  /**< TIM2 global Interrupt          */
  NVIC_IRQ_TIM3                = 29,  /**< TIM3 global Interrupt          */
  NVIC_IRQ_TIM4                = 30,  /**< TIM4 global Interrupt          */
  NVIC_IRQ_I2C1_EV             = 31,  /**< I2C1 event Interrupt           */
  NVIC_IRQ_I2C1_ER             = 32,  /**< I2C1 error Interrupt           */
  NVIC_IRQ_I2C2_EV             = 33,  /**< I2C2 event Interrupt           */
  NVIC_IRQ_I2C2_ER             = 34,  /**< I2C2 error Interrupt           */
  NVIC_IRQ_SPI1                = 35,  /**< SPI1 global Interrupt          */
  NVIC_IRQ_SPI2                = 36,  /**< SPI2 global Interrupt          */
  NVIC_IRQ_USART1              = 37,  /**< USART1 global Interrupt        */
  NVIC_IRQ_USART2              = 38,  /**< USART2 global Interrupt        */
  NVIC_IRQ_EXTI15_10           = 40,  /**< EXTI Line[15:10] Interrupts    */
  NVIC_IRQ_RTC_ALARM           = 41,  /**< RTC Alarm through EXTI Line    */
  NVIC_IRQ_OTG_FS_WKUP         = 42,  /**< USB On-The-Go FS Wakeup        */
  NVIC_IRQ_DMA1_STREAM7        = 47,  /**< DMA1 Stream7 global Interrupt  */
  NVIC_IRQ_SDIO                = 49,  /**< SDIO global Interrupt          */
  NVIC_IRQ_TIM5                = 50,  /**< TIM5 global Interrupt          */
  NVIC_IRQ_SPI3                = 51,  /**< SPI3 global Interrupt          */
  NVIC_IRQ_DMA2_STREAM0        = 56,  /**< DMA2 Stream0 global Interrupt  */
  NVIC_IRQ_DMA2_STREAM1        = 57,  /**< DMA2 Stream1 global Interrupt  */
  NVIC_IRQ_DMA2_STREAM2        = 58,  /**< DMA2 Stream2 global Interrupt  */
  NVIC_IRQ_DMA2_STREAM3        = 59,  /**< DMA2 Stream3 global Interrupt  */
  NVIC_IRQ_DMA2_STREAM4        = 60,  /**< DMA2 Stream4 global Interrupt  */
  NVIC_IRQ_OTG_FS              = 67,  /**< USB On-The-Go FS global        */
  NVIC_IRQ_DMA2_STREAM5        = 68,  /**< DMA2 Stream5 global Interrupt  */
  NVIC_IRQ_DMA2_STREAM6        = 69,  /**< DMA2 Stream6 global Interrupt  */
  NVIC_IRQ_DMA2_STREAM7        = 70,  /**< DMA2 Stream7 global Interrupt  */
  NVIC_IRQ_USART6              = 71,  /**< USART6 global Interrupt        */
  NVIC_IRQ_I2C3_EV             = 72,  /**< I2C3 event Interrupt           */
  NVIC_IRQ_I2C3_ER             = 73,  /**< I2C3 error Interrupt           */
  NVIC_IRQ_FPU                 = 81,  /**< FPU global Interrupt           */
  NVIC_IRQ_SPI4                = 84,  /**< SPI4 global Interrupt          */
  NVIC_IRQ_SPI5                = 85   /**< SPI5 global Interrupt          */
} NVIC_IRQn_t;

/* ══════════════════════════════════════════════════════════════════════
 * Priority Group Definitions (PRIGROUP)
 * ═════════════════════════════════════════════════════════════════════ */
#define NVIC_PRIORITY_GROUP_0   7U /**< 0 bits for pre-emption priority, 4 bits for subpriority */
#define NVIC_PRIORITY_GROUP_1   6U /**< 1 bits for pre-emption priority, 3 bits for subpriority */
#define NVIC_PRIORITY_GROUP_2   5U /**< 2 bits for pre-emption priority, 2 bits for subpriority */
#define NVIC_PRIORITY_GROUP_3   4U /**< 3 bits for pre-emption priority, 1 bits for subpriority */
#define NVIC_PRIORITY_GROUP_4   3U /**< 4 bits for pre-emption priority, 0 bits for subpriority */

/* ══════════════════════════════════════════════════════════════════════
 * Public API
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Enable interrupt in the NVIC.
 *
 * @param  irq  Interrupt number to enable (only non-negative).
 * @retval  0   Success.
 * @retval -1   Invalid IRQ number (negative or out of range).
 */
int nvic_enable_irq(NVIC_IRQn_t irq);

/**
 * @brief  Disable interrupt in the NVIC.
 *
 * @param  irq  Interrupt number to disable (only non-negative).
 * @retval  0   Success.
 * @retval -1   Invalid IRQ number (negative or out of range).
 */
int nvic_disable_irq(NVIC_IRQn_t irq);

/**
 * @brief  Set the pending flag for an interrupt.
 *
 * Forces the interrupt to trigger if enabled.
 *
 * @param  irq  Interrupt number to set pending (only non-negative).
 * @retval  0   Success.
 * @retval -1   Invalid IRQ number (negative or out of range).
 */
int nvic_set_pending_irq(NVIC_IRQn_t irq);

/**
 * @brief  Clear the pending flag for an interrupt.
 *
 * @param  irq  Interrupt number to clear pending (only non-negative).
 * @retval  0   Success.
 * @retval -1   Invalid IRQ number (negative or out of range).
 */
int nvic_clear_pending_irq(NVIC_IRQn_t irq);

/**
 * @brief  Read the pending flag status of an interrupt.
 *
 * @param  irq     Interrupt number to check (only non-negative).
 * @param  status  Pointer to store the status (1 = pending, 0 = not pending).
 * @retval  0      Success.
 * @retval -1      Invalid IRQ number or NULL status pointer.
 */
int nvic_get_pending_irq(NVIC_IRQn_t irq, uint8_t *status);

/**
 * @brief  Set the priority level of an interrupt or exception.
 *
 * Configures either peripheral interrupts (using NVIC->IP) or system exceptions
 * (using SCB->SHP).
 *
 * Note: STM32F411 implements only the upper 4 bits of the priority register.
 * Thus, priority ranges from 0 (highest) to 15 (lowest).
 *
 * @param  irq       Interrupt request number.
 * @param  priority  Priority level (0 to 15).
 * @retval  0        Success.
 * @retval -1        Invalid IRQ number or priority out of range (> 15).
 */
int nvic_set_priority(NVIC_IRQn_t irq, uint8_t priority);

/**
 * @brief  Get the current priority level of an interrupt or exception.
 *
 * @param  irq       Interrupt request number.
 * @param  priority  Pointer to store the priority (0 to 15).
 * @retval  0        Success.
 * @retval -1        Invalid IRQ number or NULL priority pointer.
 */
int nvic_get_priority(NVIC_IRQn_t irq, uint8_t *priority);

/**
 * @brief  Set the priority grouping grouping configuration (PRIGROUP).
 *
 * Determines the binary point split between pre-emption priority and subpriority.
 * Standard values are NVIC_PRIORITY_GROUP_0 to NVIC_PRIORITY_GROUP_4.
 *
 * @param  priority_group  Priority group value (3 to 7).
 * @retval  0              Success.
 * @retval -1              Invalid priority group value.
 */
int nvic_set_priority_grouping(uint32_t priority_group);

/**
 * @brief  Get the current priority grouping configuration.
 *
 * @return Priority group value (3 to 7).
 */
uint32_t nvic_get_priority_grouping(void);

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_NVIC_H */
