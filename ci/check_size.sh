#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────
#  check_size.sh — Verify firmware fits within STM32F411VETx limits
#
#  Usage:  ci/check_size.sh <elf-file>
#
#  Memory map (STM32F411VETx):
#    FLASH : 512 KB  (524288 bytes)  @ 0x08000000
#    RAM   : 128 KB  (131072 bytes)  @ 0x20000000
#
#  Thresholds (~93% of total):
#    FLASH : 480 KB  (491520 bytes)
#    RAM   : 120 KB  (122880 bytes)
# ──────────────────────────────────────────────────────────────────────

set -euo pipefail

ELF="${1:?Usage: $0 <elf-file>}"

# ── Limits ───────────────────────────────────────────────────────────
FLASH_TOTAL=524288     # 512 KB
RAM_TOTAL=131072       # 128 KB
FLASH_LIMIT=491520     # 480 KB  (93.75%)
RAM_LIMIT=122880       # 120 KB  (93.75%)

# ── Parse arm-none-eabi-size output ──────────────────────────────────
SIZE_OUTPUT=$(arm-none-eabi-size "$ELF")
echo "$SIZE_OUTPUT"
echo ""

# Extract: text, data, bss from the second line
#   text    data     bss     dec     hex filename
read -r TEXT DATA BSS _DEC _HEX _FILE <<< "$(arm-none-eabi-size "$ELF" | tail -1)"

# Flash = .text + .data  (code + initialized data stored in flash)
# RAM   = .data + .bss   (initialized + zero-initialized in RAM)
FLASH_USED=$((TEXT + DATA))
RAM_USED=$((DATA + BSS))

FLASH_PCT=$((FLASH_USED * 100 / FLASH_TOTAL))
RAM_PCT=$((RAM_USED * 100 / RAM_TOTAL))

echo "┌─────────────────────────────────────────────────┐"
echo "│  STM32F411VETx Memory Usage                     │"
echo "├─────────────────────────────────────────────────┤"
printf "│  Flash: %7d / %7d bytes  (%3d%%)          │\n" "$FLASH_USED" "$FLASH_TOTAL" "$FLASH_PCT"
printf "│  RAM:   %7d / %7d bytes  (%3d%%)          │\n" "$RAM_USED" "$RAM_TOTAL" "$RAM_PCT"
echo "├─────────────────────────────────────────────────┤"
printf "│  Limit: Flash < %6d KB, RAM < %3d KB        │\n" "$((FLASH_LIMIT / 1024))" "$((RAM_LIMIT / 1024))"
echo "└─────────────────────────────────────────────────┘"
echo ""

FAIL=0

if [ "$FLASH_USED" -gt "$FLASH_LIMIT" ]; then
  echo "::error::Flash usage (${FLASH_USED} bytes / ${FLASH_PCT}%) exceeds limit (${FLASH_LIMIT} bytes)!"
  FAIL=1
fi

if [ "$RAM_USED" -gt "$RAM_LIMIT" ]; then
  echo "::error::RAM usage (${RAM_USED} bytes / ${RAM_PCT}%) exceeds limit (${RAM_LIMIT} bytes)!"
  FAIL=1
fi

if [ "$FAIL" -eq 1 ]; then
  echo ""
  echo "❌ Size check FAILED"
  exit 1
fi

echo "✅ Size check passed"
