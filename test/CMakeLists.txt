# ──────────────────────────────────────────────────────────────────────
#  Unit Test build (host x86) — Unity framework
#
#  Tests run on the CI host, NOT on the ARM target.
#  Only pure-logic, data-table, and enum-validation tests belong here.
#  Hardware-dependent code is excluded from this build.
# ──────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.20)

project(stm32f411xe_tests
  LANGUAGES C
  VERSION 1.0.0
)

# ── Fetch Unity test framework ──────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG        v2.6.0
)
FetchContent_MakeAvailable(unity)

# ── Compiler settings (host build, NOT cross-compile) ───────────────
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

# ── Host compatibility: strip volatile qualifiers ───────────────────
# The project's stm32f411_xe.h does `#define __IO volatile` unconditionally.
# By defining these on the command line, the preprocessor redefines are
# harmless (macro redefinition to same value if we match, or a warning
# we suppress). We define them EMPTY so register structs compile on host.
add_compile_definitions(
  __IO=
  "__I=const"
  __O=
)

# ── Project include paths ───────────────────────────────────────────
set(PROJECT_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
set(TEST_SUPPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/support)

# ── Test executables ────────────────────────────────────────────────
enable_testing()

# Test 1: RCC prescaler tables and enum values
add_executable(test_rcc_tables test_rcc_tables.c)
target_include_directories(test_rcc_tables PRIVATE
  ${TEST_SUPPORT_DIR}
  ${PROJECT_INC_DIR}
)
target_link_libraries(test_rcc_tables PRIVATE unity)
add_test(NAME rcc_tables COMMAND test_rcc_tables)

# Test 2: GPIO enum values match register field expectations
add_executable(test_gpio_enums test_gpio_enums.c)
target_include_directories(test_gpio_enums PRIVATE
  ${TEST_SUPPORT_DIR}
  ${PROJECT_INC_DIR}
)
target_link_libraries(test_gpio_enums PRIVATE unity)
add_test(NAME gpio_enums COMMAND test_gpio_enums)

# Test 3: SPI and USART enum/config validation
add_executable(test_periph_config test_periph_config.c)
target_include_directories(test_periph_config PRIVATE
  ${TEST_SUPPORT_DIR}
  ${PROJECT_INC_DIR}
)
target_link_libraries(test_periph_config PRIVATE unity)
add_test(NAME periph_config COMMAND test_periph_config)
