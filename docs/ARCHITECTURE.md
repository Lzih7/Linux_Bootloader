# STM32 Bootloader - Architecture and Code Details

## Table of Contents

1. [System Overview](#system-overview)
2. [Bootloader Implementation](#bootloader-implementation)
3. [Application Implementation](#application-implementation)
4. [Startup Sequence](#startup-sequence)
5. [Linker Scripts Explained](#linker-scripts-explained)
6. [Memory Layout in Detail](#memory-layout-in-detail)
7. [Vector Table Management](#vector-table-management)
8. [Bootloader Jump Mechanism](#bootloader-jump-mechanism)
9. [Build System](#build-system)
10. [Debugging Guide](#debugging-guide)

## System Overview

The STM32 Bootloader project implements a two-stage firmware architecture:

```
Reset → Bootloader (0x08000000) → Application (0x08010000)
```

### Why Two Stages?

1. **Field Updates:** Bootloader stays fixed, application can be updated
2. **Recovery:** If application fails, bootloader can recover
3. **Security:** Bootloader can validate application before running
4. **Flexibility:** Multiple applications can be swapped

## Bootloader Implementation

### Entry Point: `main.c`

Located in `bootloader/src/main.c`

#### 1. System Initialization

```c
void system_init(void)
{
  /* Enable HSI (Internal 16MHz RC oscillator) */
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY));
  
  /* Reset all clock settings to default */
  RCC->CFGR = 0x00000000;
  
  /* Disable all interrupts */
  RCC->CIR = 0x00000000;
}
```

**Why HSI?**
- Internal oscillator, no external crystal needed
- Always available after reset
- Sufficient for bootloader operation

#### 2. Application Validation

```c
static int32_t validate_application(void)
{
  uint32_t app_stack_ptr = *(volatile uint32_t *)(APP_START_ADDR);
  uint32_t app_reset_handler = *(volatile uint32_t *)(APP_START_ADDR + 4);
  
  /* Validate stack pointer points to SRAM */
  if (app_stack_ptr < SRAM_BASE || app_stack_ptr > SRAM_END) {
    return BOOTLOADER_ERR_INVALID_APP;
  }
  
  /* Validate reset handler points to application flash */
  if (app_reset_handler < APP_START_ADDR || app_reset_handler > APP_END_ADDR) {
    return BOOTLOADER_ERR_INVALID_APP;
  }
  
  return BOOTLOADER_OK;
}
```

**What's Being Checked?**

The Cortex-M vector table structure:
```
Offset 0: Initial Stack Pointer (MSP)
Offset 4: Reset Handler address
Offset 8+: Interrupt handlers
```

We read the first two entries and verify:
- Stack pointer is in valid SRAM range (0x20000000-0x2000FFFF)
- Reset handler points to application flash (0x08010000-0x0803FFFF)

#### 3. The Jump

```c
static void bootloader_jump_to_app(void)
{
  typedef void (*pFunction)(void);
  
  uint32_t app_stack_ptr = *(volatile uint32_t *)(APP_START_ADDR);
  uint32_t app_reset_handler = *(volatile uint32_t *)(APP_START_ADDR + 4);
  
  /* Critical: Disable interrupts before vector table switch */
  __disable_irq();
  
  /* Set main stack pointer to application's stack */
  __set_MSP(app_stack_ptr);
  
  /* Relocate vector table */
  SCB->VTOR = APP_START_ADDR;
  
  /* Re-enable interrupts */
  __enable_irq();
  
  /* Jump to application */
  pFunction app_entry = (pFunction)app_reset_handler;
  app_entry();
}
```

**Why Disable Interrupts?**
- Prevents interrupt handlers from running during transition
- Ensures atomic state change
- Avoids calling bootloader ISRs with application context

**Why Set MSP?**
- Each program has its own stack
- Application expects its stack pointer at reset
- Prevents stack corruption

**Why Relocate VTOR?**
- Cortex-M4 has configurable vector table offset
- Bootloader: VTOR = 0x08000000
- Application: VTOR = 0x08010000
- Interrupt handlers must come from application's vector table

## Application Implementation

### Entry Point: `main.c`

Located in `application/src/main.c`

#### Critical First Step

```c
void main(void)
{
  /* MUST DO THIS FIRST! */
  __disable_irq();
  SCB->VTOR = 0x08010000;  /* Application vector table */
  __enable_irq();
  
  /* Now safe to initialize peripherals */
  system_init();
  gpio_init();
  
  /* Application loop */
  while (1) {
    led_toggle();
    delay_ms(500);
  }
}
```

**Why is This Necessary?**

When bootloader jumps to application:
1. VTOR still points to bootloader's vector table (0x08000000)
2. Application's ISRs are at 0x08010000
3. If interrupt fires before VTOR update, wrong ISR runs!
4. **Crash or undefined behavior**

## Startup Sequence

### Power-On Reset Flow

```
1. Hardware Reset
   ↓
2. CPU fetches initial SP from 0x00000000 (0x08000000)
   ↓
3. CPU fetches reset vector from 0x00000004
   ↓
4. Reset_Handler (in startup_stm32f401xc.s)
   ↓
5. Copy .data section from flash to RAM
   ↓
6. Zero .bss section
   ↓
7. Call SystemInit()
   ↓
8. Call __libc_init_array (C++ constructors)
   ↓
9. Call main()
   ↓
10. Bootloader validates application
   ↓
11. Bootloader jumps to application
   ↓
12. Application main() runs
```

### Startup File Analysis

File: `common/startup/startup_stm32f401xc.s`

#### Reset_Handler

```assembly
Reset_Handler:
  ldr   sp, =_estack      /* Set stack pointer */
  
  /* Copy .data from flash to RAM */
  movs  r1, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r3, =_sidata      /* Source: flash */
  ldr   r3, [r3, r1]
  str   r3, [r0, r1]      /* Dest: RAM */
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr   r0, =_sdata
  ldr   r3, =_edata
  adds  r2, r0, r1
  cmp   r2, r3
  bcc   CopyDataInit
  
  /* Zero .bss section */
  ldr   r2, =_sbss
  ldr   r4, =_ebss
  movs  r3, #0

FillZerobss:
  str   r3, [r2], #4
  cmp   r2, r4
  bcc   FillZerobss

  /* Call system init */
  bl  SystemInit
  
  /* Call C++ constructors */
  bl __libc_init_array
  
  /* Call main */
  bl  main
  
  /* Should never return */
  bx  lr
```

**What's Happening?**

1. **Set Stack Pointer:** Load `_estack` (top of RAM) into SP
2. **Copy .data:** Initialize variables with default values
3. **Zero .bss:** Clear uninitialized variables
4. **SystemInit:** Configure clocks, etc.
5. **Main:** Enter C code

## Linker Scripts Explained

### Bootloader Linker Script

File: `bootloader/ld/STM32F401VCTx_BOOTLOADER.ld`

```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 64K
  FLASH (rx)   : ORIGIN = 0x08000000,   LENGTH = 64K  /* Bootloader region */
}
```

**Key Sections:**

```ld
.isr_vector :
{
  . = ALIGN(4);
  KEEP(*(.isr_vector))    /* Vector table at start of flash */
  . = ALIGN(4);
} >FLASH

.text :
{
  *(.text)                /* Code */
  *(.rodata*)             /* Constants */
  _etext = .;
} >FLASH

.data :
{
  _sdata = .;
  *(.data)                /* Initialized data */
  _edata = .;
} >RAM AT> FLASH          /* Load from flash, execute in RAM */

.bss :
{
  _sbss = .;
  *(.bss)                 /* Uninitialized data */
  *(COMMON)
  _ebss = .;
} >RAM
```

**What Do These Mean?**

- `.isr_vector`: Interrupt vector table (MUST be at flash start)
- `.text`: Program code (stored in flash)
- `.data`: Initialized global variables (flash→RAM on startup)
- `.bss`: Uninitialized globals (zeroed on startup)

### Application Linker Script

File: `application/ld/STM32F401VCTx_APPLICATION.ld`

```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 64K
  FLASH (rx)   : ORIGIN = 0x08010000,   LENGTH = 192K  /* 64KB offset! */
}
```

**Critical Difference:**
- Bootloader starts at `0x08000000`
- Application starts at `0x08010000` (64KB offset)

This creates two independent firmware images!

## Memory Layout in Detail

### Flash Memory Map

```
Address    Size    Contents
--------   ----    --------------------------
0x08000000 64KB    Bootloader (fixed)
  ├─ 0x0000       Vector table
  ├─ 0x0100       Bootloader code
  └─ 0xFFFF       Bootloader end

0x08010000 192KB   Application (updatable)
  ├─ 0x0000       Vector table
  ├─ 0x0100       Application code
  └─ 0x2FFFF      Application end

0x08040000 -       Unused (256KB total)
```

### RAM Usage

```
Address    Size    Contents
--------   ----    --------------------------
0x20000000 64KB    Main SRAM
  ├─ 0x0000       Bootloader stack (top)
  ├─ 0x1000       Bootloader .data/.bss
  ├─ 0x2000       Application stack
  └─ 0x3FFF       Application .data/.bss (bottom)
```

Both bootloader and application use the same RAM, but at different times!

## Vector Table Management

### What is the Vector Table?

An array of function pointers at the start of flash:

```c
__attribute__((section(".isr_vector")))
const void* g_vector_table[] = {
  (void*)0x20010000,      // Initial Stack Pointer
  Reset_Handler,          // Reset Handler
  NMI_Handler,            // NMI Handler
  HardFault_Handler,      // Hard Fault Handler
  // ... more handlers
};
```

### VTOR Register

The **Vector Table Offset Register** (VTOR) tells the CPU where to find the vector table:

```
Bootloader mode:  SCB->VTOR = 0x08000000
Application mode: SCB->VTOR = 0x08010000
```

When an interrupt fires:
1. CPU reads VTOR
2. Adds interrupt number × 4
3. Fetches handler address
4. Jumps to handler

**This is why VTOR MUST match the active firmware!**

## Bootloader Jump Mechanism

### Detailed Step-by-Step

```c
void bootloader_jump_to_app(void) {
  // Step 1: Read application's initial state
  uint32_t app_sp = *(uint32_t*)(0x08010000 + 0x00);  // Stack pointer
  uint32_t app_pc = *(uint32_t*)(0x08010000 + 0x04);  // Reset handler
  
  // Step 2: Disable interrupts (critical!)
  __disable_irq();
  
  // Step 3: Switch to application's stack
  __set_MSP(app_sp);
  
  // Step 4: Point to application's vector table
  SCB->VTOR = 0x08010000;
  
  // Step 5: Re-enable interrupts
  __enable_irq();
  
  // Step 6: Jump to application
  ((void(*)())app_pc)();
}
```

### What Happens to the CPU?

**Before Jump:**
- PC = somewhere in bootloader
- SP = bootloader's stack
- VTOR = bootloader's vector table
- Interrupts = enabled

**After Jump:**
- PC = application's reset handler
- SP = application's stack
- VTOR = application's vector table
- Interrupts = enabled

The transition must be **atomic** - no interrupts in between!

## Build System

### Makefile Breakdown

```makefile
# Compiler
CC = arm-none-eabi-gcc

# Flags for Cortex-M4
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Include paths
C_INCLUDES = -I../bootloader/inc -I../common/cmsis/core

# Compile
$(BUILD_DIR)/%.o: %.c
    $(CC) -c $(CFLAGS) $< -o $@

# Link
$(BUILD_DIR)/bootloader.elf: $(OBJECTS)
    $(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Create binary
$(BUILD_DIR)/bootloader.bin: $(BUILD_DIR)/bootloader.elf
    arm-none-eabi-objcopy -O binary $< $@
```

### Build Script

`build.sh` orchestrates the build:

```bash
build_bootloader() {
    cd bootloader
    make clean
    make all
    cd ..
}

build_application() {
    cd application
    make clean
    make all
    cd ..
}
```

## Debugging Guide

### Using GDB with OpenOCD

**Setup:**
```bash
# Terminal 1: Start OpenOCD
openocd -f tools/openocd_stm32f4.cfg

# Terminal 2: Start GDB
arm-none-eabi-gdb bootloader/build/bootloader.elf
```

**Common GDB Commands:**

```gdb
# Connect to target
(gdb) target extended-remote :3333

# Load firmware
(gdb) load

# Set breakpoint
(gdb) break main

# Run
(gdb) continue

# Step through code
(gdb) step

# Show registers
(gdb) info registers

# Examine memory
(gdb) x/10x 0x08000000

# Show vector table
(gdb) x/10x 0x08010000
```

### Debugging the Jump

**Problem:** Application crashes after bootloader jump

**Debug Steps:**

1. **Check application vector table:**
   ```gdb
   (gdb) x/2x 0x08010000
   0x08010000:    0x20010000    0x08010189
   ```
   - First value: Stack pointer (should be in RAM)
   - Second value: Reset handler (should be in application flash)

2. **Check VTOR before jump:**
   ```gdb
   (gdb) print/x $systick
   (gdb) print/x SCB->VTOR
   ```

3. **Check SP after jump:**
   ```gdb
   (gdb) print $sp
   ```

4. **Single-step through jump:**
   ```gdb
   (gdb) break bootloader_jump_to_app
   (gdb) continue
   (gdb) stepi
   ```

### Common Issues

**Issue 1: HardFault immediately after jump**

Cause: Stack pointer or reset handler invalid

Solution: Check vector table validity
```gdb
(gdb) x/2x 0x08010000
```

**Issue 2: Interrupts crash**

Cause: VTOR not updated

Solution: Ensure VTOR is set before enabling interrupts
```c
__disable_irq();
SCB->VTOR = 0x08010000;  // MUST do this!
__enable_irq();
```

**Issue 3: Application won't start**

Cause: Bootloader validation failing

Solution: Check application is built for correct address
```bash
arm-none-eabi-objdump -h application/build/application.elf
```

Look for:
```
Idx Name          Size      VMA       LMA
  0 .isr_vector   0x000188  08010000  08010000
```

VMA should be `08010000` not `08000000`!

---

## Conclusion

This architecture provides a solid foundation for:
- **Over-the-air updates** (add wireless communication)
- **Secure boot** (add cryptographic verification)
- **Factory reset** (keep fallback image in flash)
- **Multiple applications** (switch between different firmware)

The key principles are:
1. **Isolate** bootloader and application in separate flash regions
2. **Validate** application before jumping
3. **Carefully** manage vector table and stack pointer
4. **Make** the jump atomic with interrupts disabled

Happy coding! 🚀
