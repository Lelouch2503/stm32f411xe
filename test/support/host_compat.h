/**
 * @file    host_compat.h
 * @brief   Host-build compatibility shims for STM32F411 headers
 *
 * When building unit tests on x86 (not ARM), the project headers
 * reference volatile qualifiers and memory-mapped peripheral base
 * addresses that don't exist on the host.
 *
 * The volatile stripping is handled by compiler flags in CMakeLists.txt
 * (-D__IO= -D__I=const -D__O=) so that the project's stm32f411_xe.h
 * #defines are pre-empted by the command-line definitions.
 *
 * This header provides any additional compatibility shims needed
 * for host builds.
 */

#ifndef HOST_COMPAT_H
#define HOST_COMPAT_H

/*
 * On ARM targets, inline assembly is used for dsb/isb barriers.
 * On x86 host, these are no-ops.
 */
#ifndef __arm__
#ifndef __asm
#define __asm asm
#endif
#endif

#endif /* HOST_COMPAT_H */
