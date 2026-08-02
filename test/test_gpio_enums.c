/**
 * @file    test_gpio_enums.c
 * @brief   Unit tests for GPIO enum values and configuration struct
 *
 * Validates that GPIO enum constants carry the exact register-field
 * values required by the STM32F411 hardware:
 *  - MODER[1:0] for mode
 *  - OTYPER     for output type
 *  - OSPEEDR[1:0] for speed
 *  - PUPDR[1:0]   for pull configuration
 *  - AFR[3:0]     for alternate function
 *
 * Reference: RM0383 Rev 3 — Chapter 8 (GPIO)
 */

#include "host_compat.h"
#include "stm32f411_gpio.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* ── GPIO Mode (MODER register) ──────────────────────────────────── */

void test_gpio_mode_input_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0x0, GPIO_MODE_INPUT);
}

void test_gpio_mode_output_is_1(void) {
  TEST_ASSERT_EQUAL_UINT(0x1, GPIO_MODE_OUTPUT);
}

void test_gpio_mode_altfunc_is_2(void) {
  TEST_ASSERT_EQUAL_UINT(0x2, GPIO_MODE_ALTFUNC);
}

void test_gpio_mode_analog_is_3(void) {
  TEST_ASSERT_EQUAL_UINT(0x3, GPIO_MODE_ANALOG);
}

void test_gpio_mode_fits_2_bits(void) {
  /* MODER field is 2 bits wide, so all values must fit in [0..3] */
  TEST_ASSERT_TRUE(GPIO_MODE_INPUT <= 3);
  TEST_ASSERT_TRUE(GPIO_MODE_OUTPUT <= 3);
  TEST_ASSERT_TRUE(GPIO_MODE_ALTFUNC <= 3);
  TEST_ASSERT_TRUE(GPIO_MODE_ANALOG <= 3);
}

/* ── GPIO Output Type (OTYPER register) ──────────────────────────── */

void test_gpio_otype_pushpull_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0x0, GPIO_OTYPE_PUSHPULL);
}

void test_gpio_otype_opendrain_is_1(void) {
  TEST_ASSERT_EQUAL_UINT(0x1, GPIO_OTYPE_OPENDRAIN);
}

void test_gpio_otype_fits_1_bit(void) {
  TEST_ASSERT_TRUE(GPIO_OTYPE_PUSHPULL <= 1);
  TEST_ASSERT_TRUE(GPIO_OTYPE_OPENDRAIN <= 1);
}

/* ── GPIO Speed (OSPEEDR register) ───────────────────────────────── */

void test_gpio_ospeed_low_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0x0, GPIO_OSPEED_LOW);
}

void test_gpio_ospeed_medium_is_1(void) {
  TEST_ASSERT_EQUAL_UINT(0x1, GPIO_OSPEED_MEDIUM);
}

void test_gpio_ospeed_fast_is_2(void) {
  TEST_ASSERT_EQUAL_UINT(0x2, GPIO_OSPEED_FAST);
}

void test_gpio_ospeed_high_is_3(void) {
  TEST_ASSERT_EQUAL_UINT(0x3, GPIO_OSPEED_HIGH);
}

void test_gpio_ospeed_fits_2_bits(void) {
  TEST_ASSERT_TRUE(GPIO_OSPEED_LOW <= 3);
  TEST_ASSERT_TRUE(GPIO_OSPEED_MEDIUM <= 3);
  TEST_ASSERT_TRUE(GPIO_OSPEED_FAST <= 3);
  TEST_ASSERT_TRUE(GPIO_OSPEED_HIGH <= 3);
}

/* ── GPIO Pull-up/Pull-down (PUPDR register) ─────────────────────── */

void test_gpio_pupd_none_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0x0, GPIO_PUPD_NONE);
}

void test_gpio_pupd_pullup_is_1(void) {
  TEST_ASSERT_EQUAL_UINT(0x1, GPIO_PUPD_PULLUP);
}

void test_gpio_pupd_pulldown_is_2(void) {
  TEST_ASSERT_EQUAL_UINT(0x2, GPIO_PUPD_PULLDOWN);
}

void test_gpio_pupd_fits_2_bits(void) {
  TEST_ASSERT_TRUE(GPIO_PUPD_NONE <= 3);
  TEST_ASSERT_TRUE(GPIO_PUPD_PULLUP <= 3);
  TEST_ASSERT_TRUE(GPIO_PUPD_PULLDOWN <= 3);
}

/* ── GPIO Alternate Function (AFR register) ──────────────────────── */

void test_gpio_af_values_sequential(void) {
  TEST_ASSERT_EQUAL_UINT(0x0, GPIO_AF0);
  TEST_ASSERT_EQUAL_UINT(0x1, GPIO_AF1);
  TEST_ASSERT_EQUAL_UINT(0x2, GPIO_AF2);
  TEST_ASSERT_EQUAL_UINT(0x3, GPIO_AF3);
  TEST_ASSERT_EQUAL_UINT(0x4, GPIO_AF4);
  TEST_ASSERT_EQUAL_UINT(0x5, GPIO_AF5);
  TEST_ASSERT_EQUAL_UINT(0x6, GPIO_AF6);
  TEST_ASSERT_EQUAL_UINT(0x7, GPIO_AF7);
  TEST_ASSERT_EQUAL_UINT(0x8, GPIO_AF8);
  TEST_ASSERT_EQUAL_UINT(0x9, GPIO_AF9);
  TEST_ASSERT_EQUAL_UINT(0xA, GPIO_AF10);
  TEST_ASSERT_EQUAL_UINT(0xB, GPIO_AF11);
  TEST_ASSERT_EQUAL_UINT(0xC, GPIO_AF12);
  TEST_ASSERT_EQUAL_UINT(0xD, GPIO_AF13);
  TEST_ASSERT_EQUAL_UINT(0xE, GPIO_AF14);
  TEST_ASSERT_EQUAL_UINT(0xF, GPIO_AF15);
}

void test_gpio_af_fits_4_bits(void) {
  /* AFR field is 4 bits wide, values must be [0..15] */
  TEST_ASSERT_TRUE(GPIO_AF0 <= 15);
  TEST_ASSERT_TRUE(GPIO_AF15 <= 15);
}

/* ── AF peripheral aliases match base AF values ──────────────────── */

void test_af_aliases_match_numeric(void) {
  TEST_ASSERT_EQUAL_UINT(GPIO_AF0, GPIO_AF0_SYSTEM);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF1, GPIO_AF1_TIM1_TIM2);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF2, GPIO_AF2_TIM3_TIM5);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF3, GPIO_AF3_TIM9_TIM11);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF4, GPIO_AF4_I2C1_I2C3);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF5, GPIO_AF5_SPI1_SPI5);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF6, GPIO_AF6_SPI3_SPI4);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF7, GPIO_AF7_USART1_2);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF8, GPIO_AF8_USART6);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF9, GPIO_AF9_I2C2_I2C3);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF10, GPIO_AF10_OTG_FS);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF12, GPIO_AF12_SDIO);
  TEST_ASSERT_EQUAL_UINT(GPIO_AF15, GPIO_AF15_EVENTOUT);
}

/* ── GPIO Pin numbers ────────────────────────────────────────────── */

void test_gpio_pin_numbers_sequential(void) {
  TEST_ASSERT_EQUAL_UINT(0, GPIO_PIN_0);
  TEST_ASSERT_EQUAL_UINT(1, GPIO_PIN_1);
  TEST_ASSERT_EQUAL_UINT(7, GPIO_PIN_7);
  TEST_ASSERT_EQUAL_UINT(8, GPIO_PIN_8);
  TEST_ASSERT_EQUAL_UINT(15, GPIO_PIN_15);
}

void test_gpio_pin_all(void) {
  TEST_ASSERT_EQUAL_UINT(0xFFFF, GPIO_PIN_ALL);
}

/* ── Pin state enum ──────────────────────────────────────────────── */

void test_gpio_pin_reset_is_0(void) {
  TEST_ASSERT_EQUAL_UINT(0, GPIO_PIN_RESET);
}

void test_gpio_pin_set_is_1(void) {
  TEST_ASSERT_EQUAL_UINT(1, GPIO_PIN_SET);
}

/* ── Discovery board mappings ────────────────────────────────────── */

void test_disco_led_pins(void) {
  TEST_ASSERT_EQUAL_UINT(GPIO_PIN_12, DISCO_LED_GREEN_PIN);
  TEST_ASSERT_EQUAL_UINT(GPIO_PIN_13, DISCO_LED_ORANGE_PIN);
  TEST_ASSERT_EQUAL_UINT(GPIO_PIN_14, DISCO_LED_RED_PIN);
  TEST_ASSERT_EQUAL_UINT(GPIO_PIN_15, DISCO_LED_BLUE_PIN);
}

void test_disco_button_pin(void) {
  TEST_ASSERT_EQUAL_UINT(GPIO_PIN_0, DISCO_BTN_USER_PIN);
}

/* ── GPIO_PinConfig_t struct sanity ──────────────────────────────── */

void test_gpio_pin_config_size(void) {
  /* Struct must contain all 6 fields */
  TEST_ASSERT_TRUE(sizeof(GPIO_PinConfig_t) > 0);
  /* Each field is at least an enum (int), so total ≥ 6 * sizeof(int) */
  TEST_ASSERT_TRUE(sizeof(GPIO_PinConfig_t) >= 6 * sizeof(int));
}

/* ═══════════════════════════════════════════════════════════════════ */

int main(void) {
  UNITY_BEGIN();

  /* Mode */
  RUN_TEST(test_gpio_mode_input_is_0);
  RUN_TEST(test_gpio_mode_output_is_1);
  RUN_TEST(test_gpio_mode_altfunc_is_2);
  RUN_TEST(test_gpio_mode_analog_is_3);
  RUN_TEST(test_gpio_mode_fits_2_bits);

  /* Output type */
  RUN_TEST(test_gpio_otype_pushpull_is_0);
  RUN_TEST(test_gpio_otype_opendrain_is_1);
  RUN_TEST(test_gpio_otype_fits_1_bit);

  /* Speed */
  RUN_TEST(test_gpio_ospeed_low_is_0);
  RUN_TEST(test_gpio_ospeed_medium_is_1);
  RUN_TEST(test_gpio_ospeed_fast_is_2);
  RUN_TEST(test_gpio_ospeed_high_is_3);
  RUN_TEST(test_gpio_ospeed_fits_2_bits);

  /* Pull */
  RUN_TEST(test_gpio_pupd_none_is_0);
  RUN_TEST(test_gpio_pupd_pullup_is_1);
  RUN_TEST(test_gpio_pupd_pulldown_is_2);
  RUN_TEST(test_gpio_pupd_fits_2_bits);

  /* Alternate function */
  RUN_TEST(test_gpio_af_values_sequential);
  RUN_TEST(test_gpio_af_fits_4_bits);
  RUN_TEST(test_af_aliases_match_numeric);

  /* Pin numbers */
  RUN_TEST(test_gpio_pin_numbers_sequential);
  RUN_TEST(test_gpio_pin_all);
  RUN_TEST(test_gpio_pin_reset_is_0);
  RUN_TEST(test_gpio_pin_set_is_1);

  /* Board mappings */
  RUN_TEST(test_disco_led_pins);
  RUN_TEST(test_disco_button_pin);

  /* Struct sanity */
  RUN_TEST(test_gpio_pin_config_size);

  return UNITY_END();
}
