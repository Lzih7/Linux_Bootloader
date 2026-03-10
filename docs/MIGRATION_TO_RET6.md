# STM32F401RET6 配置更新总结

## 更新内容

项目已从 **STM32F401CCU6** 更新为 **STM32F401RET6**。

## 主要区别

| 参数 | STM32F401CCU6 | STM32F401RET6 | 变化 |
|------|---------------|---------------|------|
| Flash | 256 KB | **512 KB** | +256 KB ✨ |
| SRAM | 64 KB | **96 KB** | +32 KB ✨ |
| Application空间 | 192 KB | **448 KB** | +256 KB ✨ |

## 更新的文件

### 1. 链接脚本

**bootloader/ld/STM32F401VCTx_BOOTLOADER.ld:**
```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 96K   /* 更新: 64K → 96K */
  FLASH (rx)   : ORIGIN = 0x08000000,   LENGTH = 64K   /* Bootloader区域不变 */
}
```

**application/ld/STM32F401VCTx_APPLICATION.ld:**
```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 96K   /* 更新: 64K → 96K */
  FLASH (rx)   : ORIGIN = 0x08010000,   LENGTH = 448K  /* 更新: 192K → 448K */
}
```

### 2. 头文件

**bootloader/inc/main.h:**
```c
#define APP_START_ADDR           0x08010000U  /* 不变 */
#define APP_END_ADDR             0x0807FFFFU  /* 更新: 0x0803FFFF → 0x0807FFFF */

#define SRAM_BASE                0x20000000U  /* 不变 */
#define SRAM_END                 0x20017FFFU  /* 更新: 0x2000FFFF → 0x20017FFF */
```

### 3. 文档

更新的文档文件：
- ✅ `AGENTS.md` - 编码指南
- ✅ `README.md` - 项目说明
- ✅ `docs/STM32F401RET6_CONFIG.md` - 新增配置说明

## 新的内存布局

### Flash (512 KB 总计)

```
┌─────────────────────────────────────────┐
│  Bootloader (64 KB)                     │ 0x08000000
│  ├─ Vector Table                        │
│  ├─ Bootloader Code                     │
│  └─ Bootloader Data                     │
├─────────────────────────────────────────┤
│  Application (448 KB)                   │ 0x08010000
│  ├─ Vector Table                        │
│  ├─ Application Code                    │
│  └─ Application Data                    │
└─────────────────────────────────────────┘ 0x08080000
```

### SRAM (96 KB 总计)

```
┌─────────────────────────────────────────┐
│  Main SRAM (96 KB)                      │ 0x20000000
│  ├─ Bootloader Stack                    │
│  ├─ Bootloader .data/.bss               │
│  ├─ Application Stack                   │
│  └─ Application .data/.bss              │
└─────────────────────────────────────────┘ 0x20018000
```

## 代码兼容性

✅ **完全兼容！** 所有代码无需修改，因为：

1. **启动文件相同** - startup_stm32f401xc.s 适用于所有STM32F401系列
2. **CMSIS定义相同** - 寄存器定义完全兼容
3. **外设相同** - GPIO、RCC、UART等外设相同
4. **只需要更新链接脚本** - 告诉编译器正确的内存大小

## 使用说明

### 编译（无需改变）

```bash
./build.sh all
```

### 烧录（无需改变）

```bash
openocd -f tools/openocd_stm32f4.cfg
./tools/flash_all.sh
```

### 调试（无需改变）

```bash
./tools/debug_bootloader.sh
# 或
./tools/debug_application.sh
```

## 开发板兼容性

### ✅ 完全支持

- **ST Nucleo F401RE** - 官方开发板，板载ST-Link
- **Black Pill (STM32F401RET6版本)** - 需要确认芯片型号

### LED 位置

- **Nucleo F401RE**: LD2 (绿色) 在 PA5
- **Black Pill**: 内置LED 通常在 PC13

如果LED在不同引脚，只需修改代码中的GPIO配置：

```c
// bootloader/src/main.c 和 application/src/main.c
// 将 GPIOA 改为 GPIOC
// 将 5 改为 13

RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;  // 使能 GPIOC

GPIOC->MODER &= ~(3 << (13 * 2));     // 配置 PC13
GPIOC->MODER |= (1 << (13 * 2));

GPIOC->BSRR = (1 << 13);              // 控制 PC13
```

## 验证更新

编译后检查二进制大小：

```bash
ls -lh bootloader/build/bootloader.bin
ls -lh application/build/application.bin
```

预期结果：
- bootloader.bin < 64 KB ✅
- application.bin < 448 KB ✅

查看ELF段的地址：

```bash
arm-none-eabi-objdump -h bootloader/build/bootloader.elf
arm-none-eabi-objdump -h application/build/application.elf
```

确认应用程序的VMA（虚拟内存地址）从 0x08010000 开始。

## 性能优势

更大的Flash和SRAM带来：

### ✅ 更大的应用程序空间
- 448 KB vs 192 KB (+2.3倍)
- 可以开发更复杂的应用
- 更多空间存储资源和数据

### ✅ 更多的RAM
- 96 KB vs 64 KB (+50%)
- 更大的缓冲区
- 更多的栈空间
- 更复杂的动态数据结构

## 未来扩展

有了更多空间，可以考虑：

1. **添加FreeRTOS** - 实时操作系统
2. **添加文件系统** - FatFS on SD卡
3. **添加网络协议栈** - LwIP
4. **添加USB设备栈** - USB CDC/MSC
5. **添加图形库** - LVGL或SDL
6. **添加Bootloader升级功能** - OTA更新

## 问题排查

### 编译错误

如果遇到编译错误，确保清理后重新编译：

```bash
./build.sh clean
./build.sh all
```

### 烧录问题

确认使用正确的OpenOCD配置：

```bash
openocd -f tools/openocd_stm32f4.cfg
```

### LED不闪烁

检查：
1. 芯片型号确认
2. LED引脚是否正确（Nucleo: PA5, Black Pill: PC13）
3. 使用GDB检查程序是否运行

## 总结

✅ 项目已成功迁移到STM32F401RET6
✅ 所有文件已更新
✅ 代码完全兼容
✅ 编译、烧录、调试流程不变
✅ 获得2倍以上的Flash空间
✅ 获得50%更多的RAM空间

享受更大的开发空间吧！🚀
