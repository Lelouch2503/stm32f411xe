/**
 * @file    test_rcc_tables.c
 * @brief   Unit tests for RCC prescaler tables and enum values
 *
 * Validates that:
 *  - AHB prescaler enum values match RCC_CFGR HPRE field encoding
 *  - APB prescaler enum values match RCC_CFGR PPREx field encoding
 *  - PLL-P divider enum values produce the expected division factors
 *  - Flash latency enums are sequential 0..3
 *  - System clock source enums match SW field encoding
 *  - PLL configuration parameter limits are correct
 */

#include "host_compat.h"
#include "stm32f411_rcc.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* ── AHB prescaler enum values ───────────────────────────────────── */

void test_ahb_div1_is_zero(void) {
  TEST_ASSERT_EQUAL_UINT(0x0, RCC_AHB_DIV1);
}

void test_ahb_div2_is_0x8(void) {
  TEST_ASSERT_EQUAL_UINT(0x8, RCC_AHB_DIV2);
}

void test_ahb_div4_is_0x9(void) {
  TEST_ASSERT_EQUAL_UINT(0x9, RCC_AHB_DIV4);
}

void test_ahb_div8_is_0xA(void) {
  TEST_ASSERT_EQUAL_UINT(0xA, RCC_AHB_DIV8);
}

void test_ahb_div16_is_0xB(void) {
  TEST_ASSERT_EQUAL_UINT(0xB, RCC_AHB_DIV16);
}

void test_ahb_div64_is_0xC(void) {
  TEST_ASSERT_EQUAL_UINT(0xC, RCC_AHB_DIV64);
}

void test_ahb_div128_is_0xD(void) {
  TEST_ASSERT_EQUAL_UINT(0xD, RCC_AHB_DIV128);
}

void test_ahb_div256_is_0xE(void) {
  TEST_ASSERT_EQUAL_UINT(0xE, RCC_AHB_DIV256);
}

void test_ahb_div512_is_0xF(void) {
  TEST_ASSERT_EQUAL_UINT(0xF, RCC_AHB_DIV512);
}

/* ── APB prescaler enum values ───────────────────────────────────── */

void test_apb_div1_is_zero(void) {
  TEST_ASSERT_EQUAL_UINT(0x0, RCC_APB_DIV1);
}

void test_apb_div2_is_0x4(void) {
  TEST_ASSERT_EQUAL_UINT(0x4, RCC_APB_DIV2);
}

void test_apb_div4_is_0x5(void) {
  TEST_ASSERT_EQUAL_UINT(0x5, RCC_APB_DIV4);
}

void test_apb_div8_is_0x6(void) {
  TEST_ASSERT_EQUAL_UINT(0x6, RCC_APB_DIV8);
}

void test_apb_div16_is_0x7(void) {
  TEST_ASSERT_EQUAL_UINT(0x7, RCC_APB_DIV16);
}

/* ── PLL-P divider enum values ───────────────────────────────────── */

void test_pllp_div2_is_zero(void) {
  /* PLLP register encoding: 0 = /2, 1 = /4, 2 = /6, 3 = /8 */
  TEST_ASSERT_EQUAL_UINT(0, RCC_PLLP_DIV2);
}

void test_pllp_div4_is_one(void) {
  TEST_ASSERT_EQUAL_UINT(1, RCC_PLLP_DIV4);
}

void test_pllp_div6_is_two(void) {
  TEST_ASSERT_EQUAL_UINT(2, RCC_PLLP_DIV6);
}

void test_pllp_div8_is_three(void) {
  TEST_ASSERT_EQUAL_UINT(3, RCC_PLLP_DIV8);
}

/* ── PLL-P actual division factor computation ───────────────────── */

void test_pllp_actual_divisor_formula(void) {
  /* The hardware computes actual_divisor = (PLLP_field + 1) * 2 */
  TEST_ASSERT_EQUAL_UINT(2, ((uint32_t)RCC_PLLP_DIV2 + 1U) * 2U);
  TEST_ASSERT_EQUAL_UINT(4, ((uint32_t)RCC_PLLP_DIV4 + 1U) * 2U);
  TEST_ASSERT_EQUAL_UINT(6, ((uint32_t)RCC_PLLP_DIV6 + 1U) * 2U);
  TEST_ASSERT_EQUAL_UINT(8, ((uint32_t)RCC_PLLP_DIV8 + 1U) * 2U);
}

/* ── Flash latency values ────────────────────────────────────────── */

void test_flash_latency_sequential(void) {
  TEST_ASSERT_EQUAL_UINT(0, RCC_FLASH_LATENCY_0WS);
  TEST_ASSERT_EQUAL_UINT(1, RCC_FLASH_LATENCY_1WS);
  TEST_ASSERT_EQUAL_UINT(2, RCC_FLASH_LATENCY_2WS);
  TEST_ASSERT_EQUAL_UINT(3, RCC_FLASH_LATENCY_3WS);
}

/* ── System clock source enum ────────────────────────────────────── */

void test_sysclk_src_hsi_is_zero(void) {
  TEST_ASSERT_EQUAL_UINT(0, RCC_SYSCLK_HSI);
}

void test_sysclk_src_hse_is_one(void) {
  TEST_ASSERT_EQUAL_UINT(1, RCC_SYSCLK_HSE);
}

void test_sysclk_src_pll_is_two(void) {
  TEST_ASSERT_EQUAL_UINT(2, RCC_SYSCLK_PLL);
}

/* ── PLL source enum ─────────────────────────────────────────────── */

void test_pll_src_hsi_is_zero(void) {
  TEST_ASSERT_EQUAL_UINT(0, RCC_PLLSRC_HSI);
}

void test_pll_src_hse_is_one(void) {
  TEST_ASSERT_EQUAL_UINT(1, RCC_PLLSRC_HSE);
}

/* ── Clock default values ────────────────────────────────────────── */

void test_hsi_value_is_16mhz(void) {
  TEST_ASSERT_EQUAL_UINT32(16000000U, HSI_VALUE);
}

void test_hse_value_is_8mhz(void) {
  TEST_ASSERT_EQUAL_UINT32(8000000U, HSE_VALUE);
}

/* ── Config struct size sanity ───────────────────────────────────── */

void test_pll_config_struct_not_empty(void) {
  TEST_ASSERT_TRUE(sizeof(RCC_PLL_Config_t) > 0);
}

void test_clk_init_struct_not_empty(void) {
  TEST_ASSERT_TRUE(sizeof(RCC_ClkInit_t) > 0);
}

void test_clk_init_contains_pll(void) {
  /* RCC_ClkInit_t must be at least as large as RCC_PLL_Config_t */
  TEST_ASSERT_TRUE(sizeof(RCC_ClkInit_t) >= sizeof(RCC_PLL_Config_t));
}

/* ── 100 MHz PLL parameter sanity (standard DISCO config) ────────── */

void test_standard_100mhz_pll_params(void) {
  /*
   * Standard 100 MHz config with 8 MHz HSE:
   *   PLLM=4, PLLN=100, PLLP=DIV2, PLLQ=8
   *   VCO_in  = 8 MHz / 4 = 2 MHz   (valid: 1–2 MHz)
   *   VCO_out = 2 MHz × 100 = 200 MHz (valid: 100–432 MHz)
   *   SYSCLK  = 200 MHz / 2 = 100 MHz
   *   PLL48CK = 200 MHz / 8 = 25 MHz  (not exactly 48, but valid for non-USB)
   */
  uint32_t pllm = 4;
  uint32_t plln = 100;
  uint32_t pllp_div = ((uint32_t)RCC_PLLP_DIV2 + 1U) * 2U; /* = 2 */
  uint32_t pllq = 8;

  uint32_t vco_in = HSE_VALUE / pllm;
  TEST_ASSERT_TRUE(vco_in >= 1000000U && vco_in <= 2000000U);

  uint32_t vco_out = vco_in * plln;
  TEST_ASSERT_TRUE(vco_out >= 100000000U && vco_out <= 432000000U);

  uint32_t sysclk = vco_out / pllp_div;
  TEST_ASSERT_EQUAL_UINT32(100000000U, sysclk);

  uint32_t pll48ck = vco_out / pllq;
  TEST_ASSERT_TRUE(pll48ck > 0);
  (void)pll48ck;
}

/* ═══════════════════════════════════════════════════════════════════ */

int main(void) {
  UNITY_BEGIN();

  /* AHB prescaler */
  RUN_TEST(test_ahb_div1_is_zero);
  RUN_TEST(test_ahb_div2_is_0x8);
  RUN_TEST(test_ahb_div4_is_0x9);
  RUN_TEST(test_ahb_div8_is_0xA);
  RUN_TEST(test_ahb_div16_is_0xB);
  RUN_TEST(test_ahb_div64_is_0xC);
  RUN_TEST(test_ahb_div128_is_0xD);
  RUN_TEST(test_ahb_div256_is_0xE);
  RUN_TEST(test_ahb_div512_is_0xF);

  /* APB prescaler */
  RUN_TEST(test_apb_div1_is_zero);
  RUN_TEST(test_apb_div2_is_0x4);
  RUN_TEST(test_apb_div4_is_0x5);
  RUN_TEST(test_apb_div8_is_0x6);
  RUN_TEST(test_apb_div16_is_0x7);

  /* PLL-P */
  RUN_TEST(test_pllp_div2_is_zero);
  RUN_TEST(test_pllp_div4_is_one);
  RUN_TEST(test_pllp_div6_is_two);
  RUN_TEST(test_pllp_div8_is_three);
  RUN_TEST(test_pllp_actual_divisor_formula);

  /* Flash latency */
  RUN_TEST(test_flash_latency_sequential);

  /* System clock source */
  RUN_TEST(test_sysclk_src_hsi_is_zero);
  RUN_TEST(test_sysclk_src_hse_is_one);
  RUN_TEST(test_sysclk_src_pll_is_two);

  /* PLL source */
  RUN_TEST(test_pll_src_hsi_is_zero);
  RUN_TEST(test_pll_src_hse_is_one);

  /* Clock defaults */
  RUN_TEST(test_hsi_value_is_16mhz);
  RUN_TEST(test_hse_value_is_8mhz);

  /* Struct sanity */
  RUN_TEST(test_pll_config_struct_not_empty);
  RUN_TEST(test_clk_init_struct_not_empty);
  RUN_TEST(test_clk_init_contains_pll);

  /* PLL parameter validation */
  RUN_TEST(test_standard_100mhz_pll_params);

  return UNITY_END();
}
