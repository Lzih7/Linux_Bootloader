#!/bin/bash

# Flash Bootloader Script
# This script flashes the bootloader to STM32F401 using GDB + OpenOCD

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if bootloader binary exists
if [ ! -f "bootloader/build/bootloader.elf" ]; then
    print_error "Bootloader ELF not found. Please build first:"
    echo "  ./build.sh bootloader"
    exit 1
fi

print_info "Flashing Bootloader to STM32F401..."
echo ""

# GDB commands to flash the bootloader
arm-none-eabi-gdb -batch \
    -ex "target extended-remote :3333" \
    -ex "monitor tpwr enable" \
    -ex "monitor swd_scan" \
    -ex "attach 1" \
    -ex "load bootloader/build/bootloader.elf" \
    -ex "compare-sections" \
    -ex "kill" \
    -ex "quit"

if [ $? -eq 0 ]; then
    print_info "Bootloader flashed successfully!"
    echo ""
    echo "To debug, run:"
    echo "  ./tools/debug_bootloader.sh"
else
    print_error "Flashing failed!"
    echo ""
    echo "Make sure OpenOCD is running:"
    echo "  openocd -f tools/openocd_stm32f4.cfg"
    exit 1
fi
