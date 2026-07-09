/**
 * @file    stm32f411_nvic.c
 * @brief   NVIC driver implementation for STM32F411xE
 *
 * Implements core NVIC (Nested Vectored Interrupt Controller) and
 * SCB (System Control Block) interrupt functions.
 *
 * Reference: ARMv7-M Architecture Reference Manual §B3.4
 */

#include "stm32f411_nvic.h"
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Enable / Disable Interrupts
 * ═════════════════════════════════════════════════════════════════════ */

int nvic_enable_irq(NVIC_IRQn_t irq) {
  /* Only peripheral interrupts (IRQ >= 0) can be enabled via NVIC ISER */
  if ((int32_t)irq < 0 || (int32_t)irq > 85) {
    return -1;
  }

  /* ISER is a write-1-to-set register */
  NVIC->ISER[(uint32_t)irq >> 5U] = (1U << ((uint32_t)irq & 0x1FU));
  return 0;
}

int nvic_disable_irq(NVIC_IRQn_t irq) {
  /* Only peripheral interrupts (IRQ >= 0) can be disabled via NVIC ICER */
  if ((int32_t)irq < 0 || (int32_t)irq > 85) {
    return -1;
  }

  /* ICER is a write-1-to-clear register */
  NVIC->ICER[(uint32_t)irq >> 5U] = (1U << ((uint32_t)irq & 0x1FU));
  return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Pending Interrupt Handling
 * ═════════════════════════════════════════════════════════════════════ */

int nvic_set_pending_irq(NVIC_IRQn_t irq) {
  if ((int32_t)irq < 0 || (int32_t)irq > 85) {
    return -1;
  }

  /* ISPR is a write-1-to-set-pending register */
  NVIC->ISPR[(uint32_t)irq >> 5U] = (1U << ((uint32_t)irq & 0x1FU));
  return 0;
}

int nvic_clear_pending_irq(NVIC_IRQn_t irq) {
  if ((int32_t)irq < 0 || (int32_t)irq > 85) {
    return -1;
  }

  /* ICPR is a write-1-to-clear-pending register */
  NVIC->ICPR[(uint32_t)irq >> 5U] = (1U << ((uint32_t)irq & 0x1FU));
  return 0;
}

int nvic_get_pending_irq(NVIC_IRQn_t irq, uint8_t *status) {
  if (status == NULL) {
    return -1;
  }
  if ((int32_t)irq < 0 || (int32_t)irq > 85) {
    return -1;
  }

  uint32_t reg_val = NVIC->ISPR[(uint32_t)irq >> 5U];
  *status = ((reg_val & (1U << ((uint32_t)irq & 0x1FU))) != 0U) ? 1U : 0U;
  return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Priority Configuration
 * ═════════════════════════════════════════════════════════════════════ */

int nvic_set_priority(NVIC_IRQn_t irq, uint8_t priority) {
  /* STM32F411 implements 4 priority bits, meaning values 0 to 15 */
  if (priority > 15U) {
    return -1;
  }

  if ((int32_t)irq < 0) {
    /* System exceptions priority is configured via SCB->SHP */
    int32_t sys_irq = (int32_t)irq + 12;
    if (sys_irq >= 0 && sys_irq < 12) {
      SCB->SHP[sys_irq] = (uint8_t)((priority & 0x0FU) << 4U);
      return 0;
    }
    return -1;
  } else if ((int32_t)irq <= 85) {
    /* Peripheral interrupt priority is configured via NVIC->IP */
    NVIC->IP[irq] = (uint8_t)((priority & 0x0FU) << 4U);
    return 0;
  }

  return -1;
}

int nvic_get_priority(NVIC_IRQn_t irq, uint8_t *priority) {
  if (priority == NULL) {
    return -1;
  }

  if ((int32_t)irq < 0) {
    /* Get system exception priority */
    int32_t sys_irq = (int32_t)irq + 12;
    if (sys_irq >= 0 && sys_irq < 12) {
      *priority = (uint8_t)(SCB->SHP[sys_irq] >> 4U);
      return 0;
    }
    return -1;
  } else if ((int32_t)irq <= 85) {
    /* Get peripheral interrupt priority */
    *priority = (uint8_t)(NVIC->IP[irq] >> 4U);
    return 0;
  }

  return -1;
}

/* ══════════════════════════════════════════════════════════════════════
 * Public API — Priority Grouping
 * ═════════════════════════════════════════════════════════════════════ */

int nvic_set_priority_grouping(uint32_t priority_group) {
  /* PRIGROUP supports values from 3 (4 bits preemption, 0 bits sub) to 7 (0 bits preemption, 4 bits sub) */
  if (priority_group < 3U || priority_group > 7U) {
    return -1;
  }

  uint32_t aircr = SCB->AIRCR;

  /* Clear VECTKEY and PRIGROUP fields */
  aircr &= ~(SCB_AIRCR_VECTKEY_Msk | SCB_AIRCR_PRIGROUP_Msk);

  /* Apply write key and set the new PRIGROUP value */
  aircr |= (0x05FAU << SCB_AIRCR_VECTKEY_Pos) | (priority_group << SCB_AIRCR_PRIGROUP_Pos);

  SCB->AIRCR = aircr;
  return 0;
}

uint32_t nvic_get_priority_grouping(void) {
  return (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) >> SCB_AIRCR_PRIGROUP_Pos;
}
