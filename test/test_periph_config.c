/**
 * @file    test_periph_config.c
 * @brief   Unit tests for SPI and USART enum values and configurations
 *
 * Validates that peripheral enum constants match the register-field
 * encoding expected by the STM32F411 hardware:
 *  - SPI: CPOL/CPHA modes, baud rate prescalers, bit order
 *  - USART: word length, parity, stop bits, flow control, mode, oversampling
 *
 * Reference: RM0383 Rev 3 — Chapter 20 (SPI) and Chapter 19 (USART)
 */

#include "host_compat.h"
#include "stm32f411_spi.h"
#include "stm32f411_usart.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* ══════════════════════════════════════════════════════════════════════
 * SPI Tests
 * ═════════════════════════════════════════════════════════════════════ */

/* ── SPI Mode (CPOL + CPHA) ──────────────────────────────────────── */

void test_spi_mode0_cpol0_cpha0(void) {
  /* Mode 0: CPOL=0, CPHA=0 → register value 0 */
  TEST_ASSERT_EQUAL_UINT(0, SPI_MODE_0);
}

void test_spi_mode1_cpol0_cpha1(void) {
  /* Mode 1: CPOL=0, CPHA=1 → register value 1 */
  TEST_ASSERT_EQUAL_UINT(1, SPI_MODE_1);
}

void test_spi_mode2_cpol1_cpha0(void) {
  /* Mode 2: CPOL=1, CPHA=0 → register value 2 */
  TEST_ASSERT_EQUAL_UINT(2, SPI_MODE_2);
}

void test_spi_mode3_cpol1_cpha1(void) {
  /* Mode 3: CPOL=1, CPHA=1 → register value 3 */
  TEST_ASSERT_EQUAL_UINT(3, SPI_MODE_3);
}

void test_spi_mode_cpol_cpha_decomposition(void) {
  /* CPHA is bit 0, CPOL is bit 1 of the mode value */
  TEST_ASSERT_EQUAL_UINT(0, SPI_MODE_0 & 0x1); /* CPHA=0 */
  TEST_ASSERT_EQUAL_UINT(0, SPI_MODE_0 >> 1);   /* CPOL=0 */

  TEST_ASSERT_EQUAL_UINT(1, SPI_MODE_1 & 0x1); /* CPHA=1 */
  TEST_ASSERT_EQUAL_UINT(0, SPI_MODE_1 >> 1);   /* CPOL=0 */

  TEST_ASSERT_EQUAL_UINT(0, SPI_MODE_2 & 0x1); /* CPHA=0 */
  TEST_ASSERT_EQUAL_UINT(1, SPI_MODE_2 >> 1);   /* CPOL=1 */

  TEST_ASSERT_EQUAL_UINT(1, SPI_MODE_3 & 0x1); /* CPHA=1 */
  TEST_ASSERT_EQUAL_UINT(1, SPI_MODE_3 >> 1);   /* CPOL=1 */
}

/* ── SPI Baud Rate Prescaler ─────────────────────────────────────── */

void test_spi_baudrate_dividers_sequential(void) {
  /* CR1.BR[2:0] is a 3-bit field, values 0..7 → /2, /4, ..., /256 */
  TEST_ASSERT_EQUAL_UINT(0, SPI_BAUDRATE_DIV2);
  TEST_ASSERT_EQUAL_UINT(1, SPI_BAUDRATE_DIV4);
  TEST_ASSERT_EQUAL_UINT(2, SPI_BAUDRATE_DIV8);
  TEST_ASSERT_EQUAL_UINT(3, SPI_BAUDRATE_DIV16);
  TEST_ASSERT_EQUAL_UINT(4, SPI_BAUDRATE_DIV32);
  TEST_ASSERT_EQUAL_UINT(5, SPI_BAUDRATE_DIV64);
  TEST_ASSERT_EQUAL_UINT(6, SPI_BAUDRATE_DIV128);
  TEST_ASSERT_EQUAL_UINT(7, SPI_BAUDRATE_DIV256);
}

void test_spi_baudrate_actual_divisor_formula(void) {
  /* actual_divisor = 2 << BR_field_value */
  TEST_ASSERT_EQUAL_UINT(2,   2U << SPI_BAUDRATE_DIV2);
  TEST_ASSERT_EQUAL_UINT(4,   2U << SPI_BAUDRATE_DIV4);
  TEST_ASSERT_EQUAL_UINT(8,   2U << SPI_BAUDRATE_DIV8);
  TEST_ASSERT_EQUAL_UINT(16,  2U << SPI_BAUDRATE_DIV16);
  TEST_ASSERT_EQUAL_UINT(32,  2U << SPI_BAUDRATE_DIV32);
  TEST_ASSERT_EQUAL_UINT(64,  2U << SPI_BAUDRATE_DIV64);
  TEST_ASSERT_EQUAL_UINT(128, 2U << SPI_BAUDRATE_DIV128);
  TEST_ASSERT_EQUAL_UINT(256, 2U << SPI_BAUDRATE_DIV256);
}

void test_spi_baudrate_fits_3_bits(void) {
  TEST_ASSERT_TRUE(SPI_BAUDRATE_DIV256 <= 7);
}

/* ── SPI Bit Order ───────────────────────────────────────────────── */

void test_spi_bit_order_msb_first(void) {
  /* LSBFIRST bit = 0 means MSB first (default) */
  TEST_ASSERT_EQUAL_UINT(0, SPI_BIT_ORDER_MSB_FIRST);
}

void test_spi_bit_order_lsb_first(void) {
  /* LSBFIRST bit = 1 means LSB first */
  TEST_ASSERT_EQUAL_UINT(1, SPI_BIT_ORDER_LSB_FIRST);
}

/* ── SPI Error Codes ─────────────────────────────────────────────── */

void test_spi_error_codes_are_negative(void) {
  TEST_ASSERT_TRUE(SPI_ERROR_INVALID_PARAM < 0);
  TEST_ASSERT_TRUE(SPI_ERROR_TIMEOUT < 0);
  TEST_ASSERT_TRUE(SPI_ERROR_UNSUPPORTED < 0);
  TEST_ASSERT_TRUE(SPI_ERROR_BUSY < 0);
  TEST_ASSERT_TRUE(SPI_ERROR_OVERRUN < 0);
  TEST_ASSERT_TRUE(SPI_ERROR_MODE_FAULT < 0);
}

void test_spi_error_codes_unique(void) {
  TEST_ASSERT_NOT_EQUAL(SPI_ERROR_INVALID_PARAM, SPI_ERROR_TIMEOUT);
  TEST_ASSERT_NOT_EQUAL(SPI_ERROR_TIMEOUT, SPI_ERROR_UNSUPPORTED);
  TEST_ASSERT_NOT_EQUAL(SPI_ERROR_UNSUPPORTED, SPI_ERROR_BUSY);
  TEST_ASSERT_NOT_EQUAL(SPI_ERROR_BUSY, SPI_ERROR_OVERRUN);
  TEST_ASSERT_NOT_EQUAL(SPI_ERROR_OVERRUN, SPI_ERROR_MODE_FAULT);
}

/* ── SPI Config Struct ───────────────────────────────────────────── */

void test_spi_config_struct_size(void) {
  /* Must contain mode, baudrate, bit_order */
  TEST_ASSERT_TRUE(sizeof(SPI_Config_t) >= 3 * sizeof(int));
}

/* ══════════════════════════════════════════════════════════════════════
 * USART Tests
 * ═════════════════════════════════════════════════════════════════════ */

/* ── USART Word Length (CR1.M) ───────────────────────────────────── */

void test_usart_wordlen_8bit_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0, USART_WORDLEN_8BIT);
}

void test_usart_wordlen_9bit_is_1(void) {
  TEST_ASSERT_EQUAL_UINT(1, USART_WORDLEN_9BIT);
}

/* ── USART Parity ────────────────────────────────────────────────── */

void test_usart_parity_none_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0, USART_PARITY_NONE);
}

void test_usart_parity_even_is_1(void) {
  TEST_ASSERT_EQUAL_UINT(1, USART_PARITY_EVEN);
}

void test_usart_parity_odd_is_2(void) {
  TEST_ASSERT_EQUAL_UINT(2, USART_PARITY_ODD);
}

/* ── USART Stop Bits (CR2.STOP[1:0]) ────────────────────────────── */

void test_usart_stopbits_values(void) {
  TEST_ASSERT_EQUAL_UINT(0, USART_STOPBITS_1);
  TEST_ASSERT_EQUAL_UINT(1, USART_STOPBITS_0_5);
  TEST_ASSERT_EQUAL_UINT(2, USART_STOPBITS_2);
  TEST_ASSERT_EQUAL_UINT(3, USART_STOPBITS_1_5);
}

void test_usart_stopbits_fit_2_bits(void) {
  TEST_ASSERT_TRUE(USART_STOPBITS_1_5 <= 3);
}

/* ── USART Hardware Flow Control ─────────────────────────────────── */

void test_usart_hwflow_none_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0, USART_HWFLOW_NONE);
}

void test_usart_hwflow_rts_cts(void) {
  /* RTS=bit0, CTS=bit1 → RTS_CTS = 3 */
  TEST_ASSERT_EQUAL_UINT(1, USART_HWFLOW_RTS);
  TEST_ASSERT_EQUAL_UINT(2, USART_HWFLOW_CTS);
  TEST_ASSERT_EQUAL_UINT(3, USART_HWFLOW_RTS_CTS);
}

/* ── USART Mode ──────────────────────────────────────────────────── */

void test_usart_mode_values(void) {
  /* TE=bit0, RE=bit1 */
  TEST_ASSERT_EQUAL_UINT(1, USART_MODE_TX);
  TEST_ASSERT_EQUAL_UINT(2, USART_MODE_RX);
  TEST_ASSERT_EQUAL_UINT(3, USART_MODE_TX_RX);
}

/* ── USART Oversampling ──────────────────────────────────────────── */

void test_usart_oversampling_16_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0, USART_OVERSAMPLING_16);
}

void test_usart_oversampling_8_is_1(void) {
  TEST_ASSERT_EQUAL_UINT(1, USART_OVERSAMPLING_8);
}

/* ── USART Config Struct ─────────────────────────────────────────── */

void test_usart_config_struct_size(void) {
  /* Must contain: baudrate(uint32) + 6 enum fields */
  TEST_ASSERT_TRUE(sizeof(USART_Config_t) > 0);
  TEST_ASSERT_TRUE(sizeof(USART_Config_t) >= sizeof(uint32_t) + 6 * sizeof(int));
}

/* ══════════════════════════════════════════════════════════════════════
 * Baud Rate Computation (host-side) — matches driver formula
 * ═════════════════════════════════════════════════════════════════════ */

void test_usart_brr_115200_at_50mhz_over16(void) {
  /*
   * USARTDIV = fCK / (16 × baud)
   * For PCLK1 = 50 MHz, baud = 115200, OVER8 = 0:
   *   USARTDIV = 50000000 / (16 × 115200) = 27.127
   *   Mantissa = 27, Fraction = 0.127 × 16 ≈ 2
   *   BRR = (27 << 4) | 2 = 0x1B2
   */
  uint32_t fck = 50000000U;
  uint32_t baud = 115200U;
  uint32_t over8 = 0; /* oversampling by 16 */

  uint32_t usartdiv_x100;
  if (over8 == 0) {
    usartdiv_x100 = (fck * 100U) / (16U * baud);
  } else {
    usartdiv_x100 = (fck * 100U) / (8U * baud);
  }

  uint32_t mantissa = usartdiv_x100 / 100U;
  uint32_t fraction = ((usartdiv_x100 % 100U) * 16U + 50U) / 100U;

  /* Validate mantissa and fraction are reasonable */
  TEST_ASSERT_EQUAL_UINT32(27, mantissa);
  TEST_ASSERT_TRUE(fraction <= 15);

  /* BRR value */
  uint32_t brr = (mantissa << 4) | (fraction & 0xFU);
  TEST_ASSERT_EQUAL_HEX32(0x1B2, brr);
}

void test_usart_brr_9600_at_16mhz_over16(void) {
  /*
   * HSI direct (16 MHz), baud = 9600, OVER8 = 0:
   *   USARTDIV = 16000000 / (16 × 9600) = 104.1667
   *   Mantissa = 104, Fraction = 0.1667 × 16 ≈ 3
   *   BRR = (104 << 4) | 3 = 0x683
   */
  uint32_t fck = 16000000U;
  uint32_t baud = 9600U;

  uint32_t usartdiv_x100 = (fck * 100U) / (16U * baud);
  uint32_t mantissa = usartdiv_x100 / 100U;
  uint32_t fraction = ((usartdiv_x100 % 100U) * 16U + 50U) / 100U;

  TEST_ASSERT_EQUAL_UINT32(104, mantissa);
  TEST_ASSERT_TRUE(fraction <= 15);

  uint32_t brr = (mantissa << 4) | (fraction & 0xFU);
  TEST_ASSERT_EQUAL_HEX32(0x683, brr);
}

/* ═══════════════════════════════════════════════════════════════════ */

int main(void) {
  UNITY_BEGIN();

  /* SPI Mode */
  RUN_TEST(test_spi_mode0_cpol0_cpha0);
  RUN_TEST(test_spi_mode1_cpol0_cpha1);
  RUN_TEST(test_spi_mode2_cpol1_cpha0);
  RUN_TEST(test_spi_mode3_cpol1_cpha1);
  RUN_TEST(test_spi_mode_cpol_cpha_decomposition);

  /* SPI Baud Rate */
  RUN_TEST(test_spi_baudrate_dividers_sequential);
  RUN_TEST(test_spi_baudrate_actual_divisor_formula);
  RUN_TEST(test_spi_baudrate_fits_3_bits);

  /* SPI Bit Order */
  RUN_TEST(test_spi_bit_order_msb_first);
  RUN_TEST(test_spi_bit_order_lsb_first);

  /* SPI Error Codes */
  RUN_TEST(test_spi_error_codes_are_negative);
  RUN_TEST(test_spi_error_codes_unique);

  /* SPI Config Struct */
  RUN_TEST(test_spi_config_struct_size);

  /* USART Word Length */
  RUN_TEST(test_usart_wordlen_8bit_is_0);
  RUN_TEST(test_usart_wordlen_9bit_is_1);

  /* USART Parity */
  RUN_TEST(test_usart_parity_none_is_0);
  RUN_TEST(test_usart_parity_even_is_1);
  RUN_TEST(test_usart_parity_odd_is_2);

  /* USART Stop Bits */
  RUN_TEST(test_usart_stopbits_values);
  RUN_TEST(test_usart_stopbits_fit_2_bits);

  /* USART Flow Control */
  RUN_TEST(test_usart_hwflow_none_is_0);
  RUN_TEST(test_usart_hwflow_rts_cts);

  /* USART Mode */
  RUN_TEST(test_usart_mode_values);

  /* USART Oversampling */
  RUN_TEST(test_usart_oversampling_16_is_0);
  RUN_TEST(test_usart_oversampling_8_is_1);

  /* USART Config Struct */
  RUN_TEST(test_usart_config_struct_size);

  /* Baud Rate Computation */
  RUN_TEST(test_usart_brr_115200_at_50mhz_over16);
  RUN_TEST(test_usart_brr_9600_at_16mhz_over16);

  return UNITY_END();
}
