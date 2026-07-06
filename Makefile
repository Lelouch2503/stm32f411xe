##############################################################################
#  Makefile – STM32F411E-DISCO convenience wrapper around CMake
#
#  Usage:
#    make              # Configure + Build (Debug)
#    make build        # Build only (skip configure if already done)
#    make release      # Configure + Build (Release, -Os)
#    make flash        # Build + Flash via OpenOCD
#    make clean        # Remove build directory
#    make rebuild      # Clean + Build
#    make size         # Print section sizes
#    make disasm       # Disassemble .elf to .dis
#    make openocd      # Start OpenOCD server (for debug)
#    make debug        # Start GDB + connect to OpenOCD
#    make usb-list     # List USB devices (Windows side)
#    make usb-attach   # Attach ST-Link USB to WSL via usbipd
##############################################################################

# ── Configuration ───────────────────────────────────────────────────
BUILD_DIR    := build
BUILD_TYPE   := Debug
TOOLCHAIN    := cmake/arm-none-eabi-gcc.cmake
TARGET       := stm32f411_blink
ELF          := $(BUILD_DIR)/$(TARGET).elf
BIN          := $(BUILD_DIR)/$(TARGET).bin
HEX          := $(BUILD_DIR)/$(TARGET).hex

# OpenOCD (Windows – USB is only accessible from Windows side)
OPENOCD_DIR  := /mnt/c/openocd/xpack-openocd-0.12.0-7
OPENOCD      := $(OPENOCD_DIR)/bin/openocd.exe
OCD_SCRIPTS  := $(OPENOCD_DIR)/openocd/scripts
OCD_IFACE    := interface/stlink.cfg
OCD_TARGET   := target/stm32f4x.cfg

# GDB
GDB          := gdb-multiarch

# ── Phony targets ───────────────────────────────────────────────────
.PHONY: all build release flash clean rebuild size disasm openocd debug usb-list usb-attach help

# ── Default target ──────────────────────────────────────────────────
all: build

# ── Configure + Build (Debug) ───────────────────────────────────────
$(BUILD_DIR)/Makefile:
	cmake -B $(BUILD_DIR) -G "Unix Makefiles" \
		-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: $(BUILD_DIR)/Makefile
	cmake --build $(BUILD_DIR) -- -j$$(nproc)

# ── Release build ──────────────────────────────────────────────────
release:
	cmake -B $(BUILD_DIR) -G "Unix Makefiles" \
		-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) -- -j$$(nproc)

# ── Flash via OpenOCD ───────────────────────────────────────────────
flash: build
	$(OPENOCD) -s $(OCD_SCRIPTS) -f $(OCD_IFACE) -f $(OCD_TARGET) \
		-c "program $(ELF) verify reset exit"

# ── Clean ───────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR)

# ── Rebuild (clean + build) ─────────────────────────────────────────
rebuild: clean build

# ── Print section sizes ─────────────────────────────────────────────
size: build
	arm-none-eabi-size $(ELF)

# ── Disassemble ─────────────────────────────────────────────────────
disasm: build
	arm-none-eabi-objdump -d -S $(ELF) > $(BUILD_DIR)/$(TARGET).dis
	@echo ">>> Disassembly written to $(BUILD_DIR)/$(TARGET).dis"

# ── Start OpenOCD server (keep running for debug) ───────────────────
openocd:
	$(OPENOCD) -s $(OCD_SCRIPTS) -f $(OCD_IFACE) -f $(OCD_TARGET)

# ── GDB debug session ──────────────────────────────────────────────
debug: build
	$(GDB) $(ELF) \
		-ex "target extended-remote :3333" \
		-ex "monitor reset halt" \
		-ex "load" \
		-ex "monitor reset halt" \
		-ex "break main" \
		-ex "continue"

# ── USB forwarding (usbipd-win) ─────────────────────────────────────
# WSL cannot access USB directly. Use usbipd-win to forward ST-Link.
# Run these from PowerShell (Admin) on the Windows side:
#   usbipd list
#   usbipd bind --busid <BUSID>    (one-time)
#   usbipd attach --wsl --busid <BUSID>

usb-list:
	@echo ">>> Run this in PowerShell (Admin) on Windows:"
	@echo "    usbipd list"
	@echo ""
	@echo ">>> Find your ST-Link BUSID, then run:"
	@echo "    usbipd bind --busid <BUSID>     (one-time setup)"
	@echo "    usbipd attach --wsl --busid <BUSID>"
	@echo ""
	@echo ">>> After attaching, verify in WSL with: lsusb"

usb-attach:
	@echo ">>> Checking USB devices in WSL..."
	@lsusb 2>/dev/null || echo "lsusb not found. Install: sudo apt install usbutils"

# ── Help ────────────────────────────────────────────────────────────
help:
	@echo ""
	@echo "  STM32F411E-DISCO Build System"
	@echo "  ─────────────────────────────────────────"
	@echo "  make            Build (Debug)"
	@echo "  make release    Build (Release, -Os)"
	@echo "  make flash      Build + Flash via OpenOCD"
	@echo "  make clean      Remove build directory"
	@echo "  make rebuild    Clean + Build"
	@echo "  make size       Print .elf section sizes"
	@echo "  make disasm     Disassemble .elf"
	@echo "  make openocd    Start OpenOCD server"
	@echo "  make debug      GDB debug (needs openocd running)"
	@echo "  make usb-list   Show USB attach instructions"
	@echo "  make usb-attach Check USB devices in WSL"
	@echo "  make help       Show this help"
	@echo ""
