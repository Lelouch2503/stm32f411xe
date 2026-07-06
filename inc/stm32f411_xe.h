/**
 * @file    stm32f411_xe.h
 * @brief   STM32F411xE device-level register definitions (bare-metal)
 *
 * Provides:
 *  - IO access qualifiers (__IO, __I, __O)
 *  - Memory-map base addresses
 *  - Peripheral register structures (RCC, FLASH, PWR)
 *  - Bit-field position/mask macros for all RCC-related registers
 *
 * Reference: RM0383 Rev 3 — STM32F411xC/E Reference Manual
 */

#ifndef STM32F411_XE_H
#define STM32F411_XE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ══════════════════════════════════════════════════════════════════════
 * IO Access Qualifiers
 * ═════════════════════════════════════════════════════════════════════ */
#define __IO volatile      /**< Read / Write access            */
#define __I volatile const /**< Read-only access               */
#define __O volatile       /**< Write-only access              */

/* ══════════════════════════════════════════════════════════════════════
 * Clock Source Default Values
 * ═════════════════════════════════════════════════════════════════════ */
#ifndef HSE_VALUE
#define HSE_VALUE 8000000U /**< External crystal: 8 MHz (Discovery board) */
#endif

#ifndef HSI_VALUE
#define HSI_VALUE 16000000U /**< Internal RC oscillator: 16 MHz */
#endif

/* ══════════════════════════════════════════════════════════════════════
 * Memory Map — Base Addresses
 * ═════════════════════════════════════════════════════════════════════ */
#define PERIPH_BASE 0x40000000UL

#define APB1PERIPH_BASE (PERIPH_BASE)
#define APB2PERIPH_BASE (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE (PERIPH_BASE + 0x10000000UL)

/* ── Peripheral Base Addresses ─────────────────────────────────────── */
#define PWR_BASE (APB1PERIPH_BASE + 0x7000UL)
#define RCC_BASE (AHB1PERIPH_BASE + 0x3800UL)
#define FLASH_R_BASE (AHB1PERIPH_BASE + 0x3C00UL)

/* GPIO base addresses (for future GPIO driver / raw access in main) */
#define GPIOA_BASE (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE (AHB1PERIPH_BASE + 0x1000UL)
#define GPIOH_BASE (AHB1PERIPH_BASE + 0x1C00UL)

/* ══════════════════════════════════════════════════════════════════════
 * Peripheral Register Structures
 * ═════════════════════════════════════════════════════════════════════ */

/* ── RCC (Reset and Clock Control) ─────────────────────────────────── */
typedef struct {
  __IO uint32_t CR;         /**< 0x00  Clock control                   */
  __IO uint32_t PLLCFGR;    /**< 0x04  PLL configuration               */
  __IO uint32_t CFGR;       /**< 0x08  Clock configuration             */
  __IO uint32_t CIR;        /**< 0x0C  Clock interrupt                 */
  __IO uint32_t AHB1RSTR;   /**< 0x10  AHB1 peripheral reset           */
  __IO uint32_t AHB2RSTR;   /**< 0x14  AHB2 peripheral reset           */
  uint32_t RESERVED0[2];    /**< 0x18–0x1C  Reserved                   */
  __IO uint32_t APB1RSTR;   /**< 0x20  APB1 peripheral reset           */
  __IO uint32_t APB2RSTR;   /**< 0x24  APB2 peripheral reset           */
  uint32_t RESERVED1[2];    /**< 0x28–0x2C  Reserved                   */
  __IO uint32_t AHB1ENR;    /**< 0x30  AHB1 peripheral clock enable    */
  __IO uint32_t AHB2ENR;    /**< 0x34  AHB2 peripheral clock enable    */
  uint32_t RESERVED2[2];    /**< 0x38–0x3C  Reserved                   */
  __IO uint32_t APB1ENR;    /**< 0x40  APB1 peripheral clock enable    */
  __IO uint32_t APB2ENR;    /**< 0x44  APB2 peripheral clock enable    */
  uint32_t RESERVED3[2];    /**< 0x48–0x4C  Reserved                   */
  __IO uint32_t AHB1LPENR;  /**< 0x50  AHB1 clock enable in low-power  */
  __IO uint32_t AHB2LPENR;  /**< 0x54  AHB2 clock enable in low-power  */
  uint32_t RESERVED4[2];    /**< 0x58–0x5C  Reserved                   */
  __IO uint32_t APB1LPENR;  /**< 0x60  APB1 clock enable in low-power  */
  __IO uint32_t APB2LPENR;  /**< 0x64  APB2 clock enable in low-power  */
  uint32_t RESERVED5[2];    /**< 0x68–0x6C  Reserved                   */
  __IO uint32_t BDCR;       /**< 0x70  Backup domain control           */
  __IO uint32_t CSR;        /**< 0x74  Clock control & status          */
  uint32_t RESERVED6[2];    /**< 0x78–0x7C  Reserved                   */
  __IO uint32_t SSCGR;      /**< 0x80  Spread spectrum clock gen       */
  __IO uint32_t PLLI2SCFGR; /**< 0x84  PLLI2S configuration            */
  uint32_t RESERVED7;       /**< 0x88  Reserved                        */
  __IO uint32_t DCKCFGR;    /**< 0x8C  Dedicated clocks configuration  */
} RCC_TypeDef;

/* ── FLASH Interface ───────────────────────────────────────────────── */
typedef struct {
  __IO uint32_t ACR;     /**< 0x00  Access control          */
  __IO uint32_t KEYR;    /**< 0x04  Key                     */
  __IO uint32_t OPTKEYR; /**< 0x08  Option key              */
  __IO uint32_t SR;      /**< 0x0C  Status                  */
  __IO uint32_t CR;      /**< 0x10  Control                 */
  __IO uint32_t OPTCR;   /**< 0x14  Option control          */
} FLASH_TypeDef;

/* ── PWR (Power Controller) ────────────────────────────────────────── */
typedef struct {
  __IO uint32_t CR;  /**< 0x00  Power control           */
  __IO uint32_t CSR; /**< 0x04  Power control/status    */
} PWR_TypeDef;

/* ══════════════════════════════════════════════════════════════════════
 * Peripheral Instance Macros
 * ═════════════════════════════════════════════════════════════════════ */
#define RCC ((RCC_TypeDef *)RCC_BASE)
#define FLASH ((FLASH_TypeDef *)FLASH_R_BASE)
#define PWR ((PWR_TypeDef *)PWR_BASE)

/* ══════════════════════════════════════════════════════════════════════
 * RCC Bit Definitions
 * ═════════════════════════════════════════════════════════════════════ */

/* ── RCC_CR ────────────────────────────────────────────────────────── */
#define RCC_CR_HSION_Pos (0U)
#define RCC_CR_HSION (1U << RCC_CR_HSION_Pos)
#define RCC_CR_HSIRDY_Pos (1U)
#define RCC_CR_HSIRDY (1U << RCC_CR_HSIRDY_Pos)
#define RCC_CR_HSEON_Pos (16U)
#define RCC_CR_HSEON (1U << RCC_CR_HSEON_Pos)
#define RCC_CR_HSERDY_Pos (17U)
#define RCC_CR_HSERDY (1U << RCC_CR_HSERDY_Pos)
#define RCC_CR_HSEBYP_Pos (18U)
#define RCC_CR_HSEBYP (1U << RCC_CR_HSEBYP_Pos)
#define RCC_CR_CSSON_Pos (19U)
#define RCC_CR_CSSON (1U << RCC_CR_CSSON_Pos)
#define RCC_CR_PLLON_Pos (24U)
#define RCC_CR_PLLON (1U << RCC_CR_PLLON_Pos)
#define RCC_CR_PLLRDY_Pos (25U)
#define RCC_CR_PLLRDY (1U << RCC_CR_PLLRDY_Pos)
#define RCC_CR_PLLI2SON_Pos (26U)
#define RCC_CR_PLLI2SON (1U << RCC_CR_PLLI2SON_Pos)
#define RCC_CR_PLLI2SRDY_Pos (27U)
#define RCC_CR_PLLI2SRDY (1U << RCC_CR_PLLI2SRDY_Pos)

/* ── RCC_PLLCFGR ──────────────────────────────────────────────────── */
#define RCC_PLLCFGR_PLLM_Pos (0U)
#define RCC_PLLCFGR_PLLM_Msk (0x3FU << RCC_PLLCFGR_PLLM_Pos) /* [5:0]   */
#define RCC_PLLCFGR_PLLN_Pos (6U)
#define RCC_PLLCFGR_PLLN_Msk (0x1FFU << RCC_PLLCFGR_PLLN_Pos) /* [14:6]  */
#define RCC_PLLCFGR_PLLP_Pos (16U)
#define RCC_PLLCFGR_PLLP_Msk (0x3U << RCC_PLLCFGR_PLLP_Pos) /* [17:16] */
#define RCC_PLLCFGR_PLLSRC_Pos (22U)
#define RCC_PLLCFGR_PLLSRC (1U << RCC_PLLCFGR_PLLSRC_Pos)
#define RCC_PLLCFGR_PLLSRC_HSI (0U)
#define RCC_PLLCFGR_PLLSRC_HSE (1U << RCC_PLLCFGR_PLLSRC_Pos)
#define RCC_PLLCFGR_PLLQ_Pos (24U)
#define RCC_PLLCFGR_PLLQ_Msk (0xFU << RCC_PLLCFGR_PLLQ_Pos) /* [27:24] */

/* ── RCC_CFGR ─────────────────────────────────────────────────────── */
#define RCC_CFGR_SW_Pos (0U)
#define RCC_CFGR_SW_Msk (0x3U << RCC_CFGR_SW_Pos) /* [1:0]   */
#define RCC_CFGR_SW_HSI (0x0U << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_HSE (0x1U << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SW_PLL (0x2U << RCC_CFGR_SW_Pos)

#define RCC_CFGR_SWS_Pos (2U)
#define RCC_CFGR_SWS_Msk (0x3U << RCC_CFGR_SWS_Pos) /* [3:2]   */
#define RCC_CFGR_SWS_HSI (0x0U << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_HSE (0x1U << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_SWS_PLL (0x2U << RCC_CFGR_SWS_Pos)

#define RCC_CFGR_HPRE_Pos (4U)
#define RCC_CFGR_HPRE_Msk (0xFU << RCC_CFGR_HPRE_Pos) /* [7:4]   */

#define RCC_CFGR_PPRE1_Pos (10U)
#define RCC_CFGR_PPRE1_Msk (0x7U << RCC_CFGR_PPRE1_Pos) /* [12:10] */

#define RCC_CFGR_PPRE2_Pos (13U)
#define RCC_CFGR_PPRE2_Msk (0x7U << RCC_CFGR_PPRE2_Pos) /* [15:13] */

/* ── RCC_AHB1ENR ──────────────────────────────────────────────────── */
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)
#define RCC_AHB1ENR_GPIODEN (1U << 3)
#define RCC_AHB1ENR_GPIOEEN (1U << 4)
#define RCC_AHB1ENR_GPIOHEN (1U << 7)
#define RCC_AHB1ENR_CRCEN (1U << 12)
#define RCC_AHB1ENR_DMA1EN (1U << 21)
#define RCC_AHB1ENR_DMA2EN (1U << 22)

/* ── RCC_AHB2ENR ──────────────────────────────────────────────────── */
#define RCC_AHB2ENR_OTGFSEN (1U << 7)

/* ── RCC_APB1ENR ──────────────────────────────────────────────────── */
#define RCC_APB1ENR_TIM2EN (1U << 0)
#define RCC_APB1ENR_TIM3EN (1U << 1)
#define RCC_APB1ENR_TIM4EN (1U << 2)
#define RCC_APB1ENR_TIM5EN (1U << 3)
#define RCC_APB1ENR_WWDGEN (1U << 11)
#define RCC_APB1ENR_SPI2EN (1U << 14)
#define RCC_APB1ENR_SPI3EN (1U << 15)
#define RCC_APB1ENR_USART2EN (1U << 17)
#define RCC_APB1ENR_I2C1EN (1U << 21)
#define RCC_APB1ENR_I2C2EN (1U << 22)
#define RCC_APB1ENR_I2C3EN (1U << 23)
#define RCC_APB1ENR_PWREN (1U << 28)

/* ── RCC_APB2ENR ──────────────────────────────────────────────────── */
#define RCC_APB2ENR_TIM1EN (1U << 0)
#define RCC_APB2ENR_USART1EN (1U << 4)
#define RCC_APB2ENR_USART6EN (1U << 5)
#define RCC_APB2ENR_ADC1EN (1U << 8)
#define RCC_APB2ENR_SDIOEN (1U << 11)
#define RCC_APB2ENR_SPI1EN (1U << 12)
#define RCC_APB2ENR_SPI4EN (1U << 13)
#define RCC_APB2ENR_SYSCFGEN (1U << 14)
#define RCC_APB2ENR_TIM9EN (1U << 16)
#define RCC_APB2ENR_TIM10EN (1U << 17)
#define RCC_APB2ENR_TIM11EN (1U << 18)
#define RCC_APB2ENR_SPI5EN (1U << 20)

/* ── RCC_AHB1RSTR ─────────────────────────────────────────────────── */
#define RCC_AHB1RSTR_GPIOARST (1U << 0)
#define RCC_AHB1RSTR_GPIOBRST (1U << 1)
#define RCC_AHB1RSTR_GPIOCRST (1U << 2)
#define RCC_AHB1RSTR_GPIODRST (1U << 3)
#define RCC_AHB1RSTR_GPIOERST (1U << 4)
#define RCC_AHB1RSTR_GPIOHRST (1U << 7)
#define RCC_AHB1RSTR_CRCRST (1U << 12)
#define RCC_AHB1RSTR_DMA1RST (1U << 21)
#define RCC_AHB1RSTR_DMA2RST (1U << 22)

/* ── RCC_APB1RSTR ─────────────────────────────────────────────────── */
#define RCC_APB1RSTR_TIM2RST (1U << 0)
#define RCC_APB1RSTR_TIM3RST (1U << 1)
#define RCC_APB1RSTR_TIM4RST (1U << 2)
#define RCC_APB1RSTR_TIM5RST (1U << 3)
#define RCC_APB1RSTR_WWDGRST (1U << 11)
#define RCC_APB1RSTR_SPI2RST (1U << 14)
#define RCC_APB1RSTR_SPI3RST (1U << 15)
#define RCC_APB1RSTR_USART2RST (1U << 17)
#define RCC_APB1RSTR_I2C1RST (1U << 21)
#define RCC_APB1RSTR_I2C2RST (1U << 22)
#define RCC_APB1RSTR_I2C3RST (1U << 23)
#define RCC_APB1RSTR_PWRRST (1U << 28)

/* ── RCC_APB2RSTR ─────────────────────────────────────────────────── */
#define RCC_APB2RSTR_TIM1RST (1U << 0)
#define RCC_APB2RSTR_USART1RST (1U << 4)
#define RCC_APB2RSTR_USART6RST (1U << 5)
#define RCC_APB2RSTR_ADC1RST (1U << 8)
#define RCC_APB2RSTR_SDIORST (1U << 11)
#define RCC_APB2RSTR_SPI1RST (1U << 12)
#define RCC_APB2RSTR_SPI4RST (1U << 13)
#define RCC_APB2RSTR_SYSCFGRST (1U << 14)
#define RCC_APB2RSTR_TIM9RST (1U << 16)
#define RCC_APB2RSTR_TIM10RST (1U << 17)
#define RCC_APB2RSTR_TIM11RST (1U << 18)
#define RCC_APB2RSTR_SPI5RST (1U << 20)

/* ══════════════════════════════════════════════════════════════════════
 * FLASH Bit Definitions
 * ═════════════════════════════════════════════════════════════════════ */

/* ── FLASH_ACR ─────────────────────────────────────────────────────── */
#define FLASH_ACR_LATENCY_Pos (0U)
#define FLASH_ACR_LATENCY_Msk (0xFU << FLASH_ACR_LATENCY_Pos)
#define FLASH_ACR_PRFTEN (1U << 8) /**< Prefetch enable        */
#define FLASH_ACR_ICEN (1U << 9)   /**< Instruction cache en   */
#define FLASH_ACR_DCEN (1U << 10)  /**< Data cache enable      */
#define FLASH_ACR_ICRST (1U << 11) /**< Instruction cache rst  */
#define FLASH_ACR_DCRST (1U << 12) /**< Data cache reset       */

/* ══════════════════════════════════════════════════════════════════════
 * PWR Bit Definitions
 * ═════════════════════════════════════════════════════════════════════ */

/* ── PWR_CR ────────────────────────────────────────────────────────── */
#define PWR_CR_VOS_Pos (14U)
#define PWR_CR_VOS_Msk (0x3U << PWR_CR_VOS_Pos)
#define PWR_CR_VOS_SCALE1 (0x3U << PWR_CR_VOS_Pos) /**< Max freq ≤100 MHz */
#define PWR_CR_VOS_SCALE2 (0x2U << PWR_CR_VOS_Pos) /**< Max freq ≤84 MHz  */
#define PWR_CR_VOS_SCALE3 (0x1U << PWR_CR_VOS_Pos) /**< Max freq ≤64 MHz  */

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_XE_H */
