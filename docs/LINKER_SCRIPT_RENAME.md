# 链接脚本重命名总结

## 更改内容

所有链接脚本文件已从 `STM32F401VCTx` 重命名为 `STM32F401RET6`，以准确反映目标芯片型号。

## 文件重命名

### Bootloader

```
旧名称: bootloader/ld/STM32F401VCTx_BOOTLOADER.ld
新名称: bootloader/ld/STM32F401RET6_BOOTLOADER.ld
```

### Application

```
旧名称: application/ld/STM32F401VCTx_APPLICATION.ld
新名称: application/ld/STM32F401RET6_APPLICATION.ld
```

## Makefile 更新

### bootloader/Makefile

```makefile
# 更新前
LDSCRIPT = ../bootloader/ld/STM32F401VCTx_BOOTLOADER.ld

# 更新后
LDSCRIPT = ../bootloader/ld/STM32F401RET6_BOOTLOADER.ld
```

### application/Makefile

```makefile
# 更新前
LDSCRIPT = ../application/ld/STM32F401VCTx_APPLICATION.ld

# 更新后
LDSCRIPT = ../application/ld/STM32F401RET6_APPLICATION.ld
```

## 验证

所有更改已完成并验证：

✅ 链接脚本文件已重命名
✅ Bootloader Makefile 已更新
✅ Application Makefile 已更新
✅ 文件引用正确

## 影响范围

这些更改仅影响文件名，不影响：
- 链接脚本内容（已经是正确的STM32F401RET6配置）
- 编译过程
- 生成的二进制文件
- 功能逻辑

## 下一步

可以正常进行编译和烧录：

```bash
# 编译
./build.sh all

# 烧录
openocd -f tools/openocd_stm32f4.cfg
./tools/flash_all.sh
```

## 芯片型号说明

- **STM32F401VCTx**: 通常指STM32F401VCT6（256KB Flash）
- **STM32F401RET6**: 512KB Flash，96KB SRAM（本项目使用的型号）

使用正确的文件名可以避免混淆，明确项目针对的芯片型号。
