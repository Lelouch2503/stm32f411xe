/**
 * @file    stm32f411_usart.h
 * @brief   USART driver for STM32F411xE — public API
 *
 * Provides functions to:
 *  - Configure USART peripherals (baud rate, word length, parity, stop bits)
 *  - Transmit and receive data in polling mode with timeout
 *  - Auto-configure GPIO pins for known USART instances
 *
 * Supported instances: USART1, USART2, USART6
 *
 * Default pin mapping (auto-configured by usart_init):
 *   USART1 — PA9  (TX, AF7), PA10 (RX, AF7)
 *   USART2 — PA2  (TX, AF7), PA3  (RX, AF7)
 *   USART6 — PA11 (TX, AF8), PA12 (RX, AF8)
 *
 * Reference: RM0383 Rev 3 — STM32F411xC/E Reference Manual, Chapter 19
 */

#ifndef STM32F411_USART_H
#define STM32F411_USART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f411_xe.h"

/* ══════════════════════════════════════════════════════════════════════
 * Enumerations — USART Configuration Options
 * ═════════════════════════════════════════════════════════════════════ */

/** USART word length (CR1.M) */
typedef enum {
  USART_WORDLEN_8BIT = 0U, /**< 8 data bits */
  USART_WORDLEN_9BIT = 1U  /**< 9 data bits */
} USART_WordLength_t;

/** USART parity selection (CR1.PCE + CR1.PS) */
typedef enum {
  USART_PARITY_NONE = 0U, /**< No parity                         */
  USART_PARITY_EVEN = 1U, /**< Even parity (PCE=1, PS=0)         */
  USART_PARITY_ODD  = 2U  /**< Odd parity  (PCE=1, PS=1)         */
} USART_Parity_t;

/** USART stop bits (CR2.STOP[1:0]) */
typedef enum {
  USART_STOPBITS_1   = 0U, /**< 1 stop bit                       */
  USART_STOPBITS_0_5 = 1U, /**< 0.5 stop bits                    */
  USART_STOPBITS_2   = 2U, /**< 2 stop bits                      */
  USART_STOPBITS_1_5 = 3U  /**< 1.5 stop bits                    */
} USART_StopBits_t;

/** USART hardware flow control (CR3.RTSE + CR3.CTSE) */
typedef enum {
  USART_HWFLOW_NONE    = 0U, /**< No flow control                */
  USART_HWFLOW_RTS     = 1U, /**< RTS enabled                    */
  USART_HWFLOW_CTS     = 2U, /**< CTS enabled                    */
  USART_HWFLOW_RTS_CTS = 3U  /**< RTS + CTS enabled              */
} USART_HwFlowCtl_t;

/** USART mode — transmitter/receiver enable (CR1.TE + CR1.RE) */
typedef enum {
  USART_MODE_TX    = 1U, /**< Transmitter only                   */
  USART_MODE_RX    = 2U, /**< Receiver only                      */
  USART_MODE_TX_RX = 3U  /**< Transmitter + Receiver             */
} USART_Mode_t;

/** USART oversampling mode (CR1.OVER8) */
typedef enum {
  USART_OVERSAMPLING_16 = 0U, /**< Oversampling by 16            */
  USART_OVERSAMPLING_8  = 1U  /**< Oversampling by 8             */
} USART_OverSampling_t;

/* ══════════════════════════════════════════════════════════════════════
 * Configuration Structure
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief USART configuration parameters.
 *
 * Baud rate formula (RM0383 §19.3.4):
 *   OVER8=0: USARTDIV = fCK / (16 × baud)
 *   OVER8=1: USARTDIV = fCK / (8  × baud)
 */
typedef struct {
  uint32_t             baudrate;       /**< Desired baud rate (e.g. 115200)  */
  USART_WordLength_t   word_length;    /**< Data bits: 8 or 9               */
  USART_Parity_t       parity;         /**< Parity: None, Even, Odd          */
  USART_StopBits_t     stop_bits;      /**< Stop bits: 1, 0.5, 2, 1.5       */
  USART_HwFlowCtl_t    hw_flow_ctl;    /**< Hardware flow control            */
  USART_Mode_t         mode;           /**< TX, RX, or TX+RX                 */
  USART_OverSampling_t oversampling;   /**< 16x or 8x oversampling           */
} USART_Config_t;

/* ══════════════════════════════════════════════════════════════════════
 * Error Codes
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * USART driver error codes (returned by public API functions):
 *
 *   0  — Success
 *  -1  — Invalid parameter (NULL pointer, unsupported instance, etc.)
 *  -2  — Timeout (hardware not ready within expected iterations)
 *  -3  — Unsupported configuration (e.g. baud rate impossible with clock)
 *  -5  — Frame/noise/parity/overrun error detected during receive
 */

/* ══════════════════════════════════════════════════════════════════════
 * Public API
 * ═════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Initialize a USART peripheral.
 *
 * Sequence:
 *   1. Enable peripheral clock (APB1 or APB2 depending on instance)
 *   2. Configure GPIO TX/RX pins as alternate function push-pull
 *   3. Disable UE before configuration
 *   4. Set word length, parity, stop bits, mode, oversampling, flow control
 *   5. Calculate and set baud rate register (BRR)
 *   6. Clear stale SR flags
 *   7. Enable the USART peripheral (UE)
 *
 * @param  instance  USART peripheral (USART1, USART2, or USART6).
 * @param  config    Pointer to configuration structure.
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 * @retval -3   Unsupported configuration (baud rate yields zero BRR).
 */
int usart_init(USART_TypeDef *instance, const USART_Config_t *config);

/**
 * @brief  De-initialize a USART peripheral.
 *
 * Disables the peripheral and resets it via RCC.
 *
 * @param  instance  USART peripheral (USART1, USART2, or USART6).
 * @retval  0   Success.
 * @retval -1   Invalid parameter (unsupported instance).
 */
int usart_deinit(USART_TypeDef *instance);

/**
 * @brief  Transmit a buffer of bytes in polling mode.
 *
 * Waits for TXE before writing each byte. After all bytes are sent,
 * waits for TC (Transmission Complete) before returning.
 *
 * @param  instance  USART peripheral.
 * @param  data      Pointer to transmit buffer.
 * @param  len       Number of bytes to transmit.
 * @param  timeout   Maximum polling iterations per byte (0 = infinite).
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 * @retval -2   Timeout waiting for TXE or TC.
 */
int usart_transmit(USART_TypeDef *instance, const uint8_t *data,
                   uint32_t len, uint32_t timeout);

/**
 * @brief  Receive a buffer of bytes in polling mode.
 *
 * Waits for RXNE before reading each byte. Checks for hardware
 * error flags (ORE, FE, NF, PE) and returns an error if any are set.
 *
 * @param  instance  USART peripheral.
 * @param  data      Pointer to receive buffer.
 * @param  len       Number of bytes to receive.
 * @param  timeout   Maximum polling iterations per byte (0 = infinite).
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 * @retval -2   Timeout waiting for RXNE.
 * @retval -5   USART hardware error (ORE/FE/NF/PE).
 */
int usart_receive(USART_TypeDef *instance, uint8_t *data,
                  uint32_t len, uint32_t timeout);

/**
 * @brief  Transmit a single byte in polling mode.
 *
 * @param  instance  USART peripheral.
 * @param  byte      Byte to transmit.
 * @param  timeout   Maximum polling iterations (0 = infinite).
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 * @retval -2   Timeout.
 */
int usart_write_byte(USART_TypeDef *instance, uint8_t byte, uint32_t timeout);

/**
 * @brief  Receive a single byte in polling mode.
 *
 * @param  instance  USART peripheral.
 * @param  byte      Pointer to store received byte.
 * @param  timeout   Maximum polling iterations (0 = infinite).
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 * @retval -2   Timeout.
 * @retval -5   USART hardware error.
 */
int usart_read_byte(USART_TypeDef *instance, uint8_t *byte, uint32_t timeout);

/**
 * @brief  Transmit a null-terminated string in polling mode.
 *
 * @param  instance  USART peripheral.
 * @param  str       Null-terminated string to transmit.
 * @param  timeout   Maximum polling iterations per byte (0 = infinite).
 * @retval  0   Success.
 * @retval -1   Invalid parameter.
 * @retval -2   Timeout.
 */
int usart_puts(USART_TypeDef *instance, const char *str, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_USART_H */
