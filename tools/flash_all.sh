#!/bin/bash

# Flash Both Bootloader and Application
# This script flashes both bootloader and application to STM32F401

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

print_info "=========================================="
print_info "Flashing Bootloader + Application"
print_info "=========================================="
echo ""

# Flash bootloader
print_info "Step 1/2: Flashing Bootloader..."
./tools/flash_bootloader.sh
if [ $? -ne 0 ]; then
    print_error "Bootloader flash failed!"
    exit 1
fi
echo ""

# Flash application
print_info "Step 2/2: Flashing Application..."
./tools/flash_application.sh
if [ $? -ne 0 ]; then
    print_error "Application flash failed!"
    exit 1
fi
echo ""

print_info "=========================================="
print_info "Flash Complete!"
print_info "=========================================="
echo ""
print_info "What to expect:"
echo "  1. Reset the board (press NRST button)"
echo "  2. LED will blink FAST (2Hz) - Bootloader running"
echo "  3. After ~1 second, LED will blink SLOW (0.5Hz) - Application running"
echo ""
print_info "LED Behavior Guide:"
echo "  - Fast blink (2Hz):     Bootloader is running, checking application"
echo "  - Slow blink (0.5Hz):   Application is running successfully"
echo "  - Solid on:            Error - No valid application found"
echo "  - Off:                 Hardware/Power issue"
echo ""
