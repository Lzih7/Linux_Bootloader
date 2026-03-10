# STM32 Bootloader 快速开始指南

本指南将帮助您快速搭建和运行STM32F401 Bootloader项目。

## 硬件准备

- STM32F401CCU6开发板（如Black Pill）
- ST-Link调试器（板载或外接）
- Micro USB数据线

## 软件安装

### 1. 安装工具链

在Linux/WSL终端中执行：

```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi openocd
```

### 2. 验证安装

```bash
arm-none-eabi-gcc --version
openocd --version
```

应该看到版本号输出（gcc 10.3+, openocd 0.11+）。

## 编译项目

### 编译全部（Bootloader + Application）

```bash
./build.sh all
```

编译成功后，生成以下文件：
- `bootloader/build/bootloader.bin` - 引导程序
- `application/build/application.bin` - 应用程序

### 单独编译

```bash
# 只编译Bootloader
./build.sh bootloader

# 只编译Application
./build.sh application

# 清理编译文件
./build.sh clean
```

## 烧录到单片机

### 方法1：使用一键烧录脚本（推荐）

**第一步：启动OpenOCD**

打开第一个终端，运行：

```bash
openocd -f tools/openocd_stm32f4.cfg
```

保持这个终端运行！

**第二步：烧录程序**

打开第二个终端，运行：

```bash
./tools/flash_all.sh
```

这会将bootloader和application都烧录到单片机。

### 方法2：单独烧录

如果只烧录bootloader：

```bash
./tools/flash_bootloader.sh
```

如果只烧录application：

```bash
./tools/flash_application.sh
```

## 运行程序

烧录完成后：

1. **按开发板上的RESET按钮**
2. 观察LED指示灯

### 预期行为

```
时间    LED状态          说明
------  ---------------  --------------------------
0s      快速闪烁 (2Hz)    Bootloader运行中
1s      慢速闪烁 (0.5Hz)  Application运行中
```

### LED状态说明

| LED状态 | 含义 |
|---------|------|
| 快速闪烁（每秒2次） | Bootloader正在验证应用程序 |
| 慢速闪烁（每秒0.5次） | 应用程序正常运行 |
| 常亮 | 错误：没有有效的应用程序 |
| 不亮 | 硬件或电源问题 |

## 调试

### 调试Bootloader

**终端1：启动OpenOCD**
```bash
openocd -f tools/openocd_stm32f4.cfg
```

**终端2：启动GDB**
```bash
./tools/debug_bootloader.sh
```

### 调试Application

**终端1：启动OpenOCD**
```bash
openocd -f tools/openocd_stm32f4.cfg
```

**终端2：启动GDB**
```bash
./tools/debug_application.sh
```

### GDB常用命令

```
load              - 加载程序到单片机
break main        - 在main函数设置断点
continue          - 继续运行
step/next         - 单步执行
info registers    - 显示寄存器
print 变量名       - 打印变量值
x/10x 0x08000000  - 查看指定地址的内存
quit              - 退出GDB
```

## 常见问题

### Q1: 提示找不到arm-none-eabi-gcc

**A:** 工具链未安装，运行：
```bash
sudo apt-get install gcc-arm-none-eabi
```

### Q2: OpenOCD连接失败

**A:** 检查：
1. ST-Link驱动是否正确安装
2. USB线是否连接
3. 运行 `lsusb | grep STMicro` 查看是否识别到ST-Link

### Q3: 应用程序不启动，LED一直快速闪烁

**A:** Bootloader无法找到有效的应用程序。检查：
1. application是否正确编译
2. application是否烧录到正确地址（0x08010000）
3. 使用以下命令检查：
```bash
arm-none-eabi-objdump -h application/build/application.elf
```

### Q4: 编译出错

**A:** 清理后重新编译：
```bash
./build.sh clean
./build.sh all
```

## 项目结构说明

```
Linux_Bootloader/
├── bootloader/          # 引导程序代码
│   ├── src/            # 源文件
│   ├── inc/            # 头文件
│   ├── ld/             # 链接脚本
│   └── Makefile        # 编译文件
├── application/         # 应用程序代码
│   ├── src/
│   ├── inc/
│   ├── ld/
│   └── Makefile
├── common/              # 公共文件
│   ├── startup/        # 启动代码
│   └── cmsis/          # CMSIS头文件
├── tools/               # 工具脚本
│   ├── flash_*.sh      # 烧录脚本
│   └── debug_*.sh      # 调试脚本
├── build.sh             # 主编译脚本
└── README.md            # 说明文档
```

## 内存布局

STM32F401CCU6的Flash（256KB）被分成两个区域：

```
地址           大小    用途
-------------  ------  ------------------
0x08000000     64KB    Bootloader（固定）
0x08010000     192KB   Application（可更新）
```

## 下一步

- 阅读 `README.md` 了解完整功能
- 阅读 `docs/ARCHITECTURE.md` 了解架构细节
- 阅读 `AGENTS.md` 了解代码规范
- 修改 `application/src/main.c` 添加自己的功能
- 修改 `bootloader/src/main.c` 增强Bootloader功能

## 技术支持

遇到问题？

1. 查看 README.md 的故障排除部分
2. 查看 ARCHITECTURE.md 的调试指南
3. 检查STM32参考手册 RM0368
4. 查看OpenOCD和GDB文档

## 参考资源

- [STM32F401参考手册](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbxc-and-stm32f401xdxe-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [ARM Cortex-M4编程手册](https://developer.arm.com/documentation/ddi0439/latest/)
- [GNU LD链接脚本](https://sourceware.org/binutils/docs/ld/Scripts.html)

---

祝您使用愉快！ 🎉
