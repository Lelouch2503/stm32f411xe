cmake_minimum_required(VERSION 3.20)

# ── Project ──────────────────────────────────────────────────────────
project(stm32f411xe
    LANGUAGES C ASM
    VERSION   1.0.0
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ── MCU / Target flags ──────────────────────────────────────────────
set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

# Linker script (absolute path so out-of-tree builds work)
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F411VETx.ld)

# ── Compiler flags ──────────────────────────────────────────────────
add_compile_options(
    ${MCU_FLAGS}
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
    -fno-common
)

# ASM-specific flags
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp")

# Build-type specific flags
set(CMAKE_C_FLAGS_DEBUG   "-Og -g3 -gdwarf-2 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")

# Default to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ── Sources ─────────────────────────────────────────────────────────
set(SOURCES
    main.c
    startup/startup_stm32f411ve.s
    startup/rtos_port.s

    src/rtos_ipc.c
    src/rtos_kernel.c
    src/rtos_port.c
    src/stm32f411_dma.c
    src/stm32f411_gpio.c
    src/stm32f411_i2c.c
    src/stm32f411_nvic.c
    src/stm32f411_rcc.c
    src/stm32f411_spi.c
    src/stm32f411_systick.c
    src/stm32f411_usart.c
)

# # Driver source files (src/*.c)
# file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.c)
# list(APPEND SOURCES ${SRC_FILES})

# ── Include directories ─────────────────────────────────────────────
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/inc
)

add_library(cmsis_core INTERFACE)

target_include_directories(cmsis_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/Common
)

# ── Executable target ───────────────────────────────────────────────
set(TARGET ${PROJECT_NAME})
set(ELF_FILE ${TARGET}.elf)
set(BIN_FILE ${TARGET}.bin)
set(HEX_FILE ${TARGET}.hex)
set(MAP_FILE ${TARGET}.map)

add_executable(${ELF_FILE} ${SOURCES})

target_include_directories(${ELF_FILE} PRIVATE
    ${INCLUDE_DIRS}
)

target_link_libraries(${ELF_FILE} PRIVATE
    cmsis_core
)

# ── Linker flags ────────────────────────────────────────────────────
target_link_options(${ELF_FILE} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${MAP_FILE},--cref
    --specs=nosys.specs
    --specs=nano.specs
    -nostartfiles
)

# ── Post-build: generate .bin, .hex and print size ──────────────────
add_custom_command(TARGET ${ELF_FILE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILE} ${BIN_FILE}
    COMMAND ${CMAKE_OBJCOPY} -O ihex   ${ELF_FILE} ${HEX_FILE}
    COMMAND ${CMAKE_SIZE}    ${ELF_FILE}
    COMMENT ">>> Generating ${BIN_FILE} and ${HEX_FILE}"
)

# ── Flash target (OpenOCD) ──────────────────────────────────────────
add_custom_target(flash
    COMMAND openocd
        -f interface/stlink.cfg
        -f target/stm32f4x.cfg
        -c "program ${ELF_FILE} verify reset exit"
    DEPENDS ${ELF_FILE}
    COMMENT ">>> Flashing ${ELF_FILE} via OpenOCD + ST-Link"
)

# ── Documentation target (Doxygen) ──────────────────────────────────
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT ">>> Generating Doxygen documentation"
        VERBATIM
    )
endif()
