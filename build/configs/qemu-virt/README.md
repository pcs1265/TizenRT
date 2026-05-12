# QEMU Virtual Platform (qemu-virt)

This configuration is for running TizenRT on the QEMU `virt` machine for the ARM architecture. It allows for testing and development of the OS without needing physical hardware.

## Overview

### Key Features
- **Virtual Environment**: Complete development environment using software only, no physical hardware required
- **Fast Test Cycle**: Rapid testing and debugging without hardware reboot
- **DRAM Boot (dramboot)**: BL1 starts from pflash, then loads the kernel and protected binaries from the virtio-blk image into the DRAM boot flow

### Configurations

This directory contains the following pre-defined configurations for the `qemu-virt` board:

| Configuration | Build Model | CPU | Binary Format | Description |
|---------------|-------------|-----|---------------|-------------|
| `dramboot_flat` | Flat (`CONFIG_BUILD_FLAT=y`) | SMP (4 cores) | - | Simple build model where OS and applications are linked into a single binary |
| `dramboot_elf` | Protected (`CONFIG_BUILD_PROTECTED=y`) | Single Core | ELF (`CONFIG_ELF=y`) | **Recommended for Beginners** - Kernel and loadable binaries are prepared for virtio-blk-backed dramboot |
| `dramboot_elf_smp` | Protected (`CONFIG_BUILD_PROTECTED=y`) | SMP (4 cores) | ELF (`CONFIG_ELF=y`) | SMP-enabled protected build |
| `dramboot_elf_net` | Protected (`CONFIG_BUILD_PROTECTED=y`) | Single Core | ELF (`CONFIG_ELF=y`) | Protected build with virtio-net enabled |


## Quick Start Guide

### Step 1: Configure and Build

```bash
# Navigate to the os directory
cd os

# Configure for the protected ELF flow (recommended for beginners)
./dbuild.sh configure qemu-virt dramboot_elf

# Build the system
./dbuild.sh
```

### Step 2: Download Binaries to QEMU Flash Image

```bash
# Run in os directory
# download to Partition A
./dbuild.sh download bl1 kernel common app1 bootparam
# or interactive partition selection
./dbuild.sh download ALL
```

If needed, `qemu_flash.bin` will be created in project root directory.
Download process copies `run_qemu.sh` that required in next step to project root directory.

### Step 3: Run QEMU

```bash
# Run in project root directory
./run_qemu.sh
```

## Machine Components

### Hardware Components

#### PL011 UART Controller
**Description**: ARM PrimeCell PL011 UART controller for serial communication.

**Specifications**:
- **Base Address**: 0x09000000
- **Interrupt**: IRQ 33
- **Clock Frequency**: 24MHz
- **Function**: Serial console communication and bootloader output

#### Virtio-MMIO Devices
**Description**: Virtual I/O framework for efficient paravirtualized device access with customizable peripheral support.

**Specifications**:
- **Base Address**: 0x0A000000
- **Device Spacing**: 0x200 bytes
- **Maximum Devices**: 32
- **Supported Devices**: Virtio Block Device (ID: 2), Network Device, Console, RNG, and more
- **Function**: High-performance device emulation with flexible peripheral configuration
- **Customization**: Users can add various virtio devices for testing different hardware scenarios

### Memory Architecture

The qemu-virt platform uses the following memory map:

| Region | Address Range | Size | Description |
|--------|--------------|------|-------------|
| Flash 0 | 0x00000000-0x04000000 | 64MB | Primary boot and application storage |
| Flash 1 | 0x04000000-0x08000000 | 64MB | Secondary boot and application storage |
| I/O | 0x08000000-0x0e000000 | 96MB | PL011 UART, Virtio devices |
| Secure Memory | 0x0e000000-0x0f000000 | 16MB | Secure world memory |
| PCIe | 0x10000000-0x40000000 | 768MB | PCIe device space |
| DDR | 0x40000000-0x50000000 | 256MB | System RAM |

### Software Components

#### S1-boot (BL1)
**Description**: QEMU-virt specific bootloader, the first bootloader executed during system boot.

**Role**: This bootloader handles:
- Hardware initialization
- Boot parameter and kernel partition checksum verification
- RAM initialization
- Kernel loading and execution

#### Virtio Driver Stack (WIP)
**Description**: Virtio implementation for qemu-virt board.

**Components**:
- **virtio-mmio**: Memory-mapped Virtio device interface
- **virtio-blk**: Block device driver implementation
- **virtio-queue**: Virtqueue management for efficient I/O

#### PL011 UART Driver
**Description**: Serial communication driver for qemu-virt platform.

**Features**:
- FIFO-based transmission and reception
- Configurable baud rates and data formats
- Interrupt-driven I/O operations
- Console support for system interaction

## Debugging

This section explains how to debug TizenRT on QEMU using Visual Studio Code.

### 1. Prerequisites

- **Visual Studio Code**: Install from the [official website](https://code.visualstudio.com/).
- **Cortex-Debug Extension**: Install the `Cortex-Debug` extension from the Visual Studio Code Marketplace. You can search for `marlonbaeten.cortex-debug` in the Extensions view (Ctrl+Shift+X).
- **ARM Toolchain or gdb-multiarch**: The `gdb-multiarch` debugger is required. Ensure `gdb-multiarch` is installed and accessible in your system's PATH. You may need to install it via your system's package manager (e.g., `sudo apt install gdb-multiarch` on Debian/Ubuntu).
- **Docker**: The QEMU environment is run inside a Docker container. Ensure Docker is installed and running on your system.

### 2. Setup

1. **Create `.vscode` directory**: At the root of your TizenRT project, create a directory named `.vscode`.

    ```bash
    mkdir -p .vscode
    ```

2. **Copy `launch.json`**: Copy the debugger configuration file to the newly created directory.

    ```bash
    cp build/configs/qemu-virt/tools/launch.json .vscode/launch.json
    ```

    *Note: Remember to edit `.vscode/launch.json` if your `arm-none-eabi-gdb` path is different.*

### 3. Debugging Session

1. **Start QEMU with GDB Server**:
    Run the `run_qemu.sh` script. This script will start QEMU inside a Docker container and automatically start a GDB server listening on port `1234`, which is exposed to your host machine.

    ```bash
    ./run_qemu.sh
    ```

2. **Start Debugging in VS Code**:
    - Open your TizenRT project folder in Visual Studio Code.
    - Go to the "Run and Debug" view on the left-hand side (or press Ctrl+Shift+D).
    - From the dropdown menu at the top, select the desired debug configuration:
      - **"QEMU(S1-Boot)"**: Debug the bootloader
      - **"QEMU(Kernel)"**: Debug the kernel only (Flat)
      - **"QEMU(ELF)"**: Debug the kernel and standard ELF applications (Load symbols automatically : check `build/configs/qemu-virt/tools/auto_symbol_loader.py`)
    - Press the "Start Debugging" button (the green play icon) or press F5.

The VS Code debugger should now attach to the QEMU session, and you can start debugging your code.

## Limitations
   - In the current SMP configuration, TizenRT uses WFE in the idle loop on all CPUs except CPU 0.
     - QEMU emulates WFE as a busy wait, which causes host CPU usage to ramp up to 100%.
     - This issue can be resolved by supporting a custom idle loop in SMP.
   - BL1 still loads from pflash, while kernel and protected binaries are stored in the virtio-blk image.
