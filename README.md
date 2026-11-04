# IROK AL87 Pro — BK2535 2.4GHz USB Dongle Firmware

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/MCU-Beken%20BK2535%20(MCS--51)-blue.svg)]()
[![Compiler](https://img.shields.io/badge/compiler-SDCC-orange.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

This repository contains the 2.4GHz USB Dongle receiver firmware for the **IROK AL87 Pro** mechanical keyboard, targeting the **Beken BK2535** (MCS-51 / 8051) microcontroller.

It is paired with the keyboard's [BK3632 wireless co-processor](https://github.com/aierm/irok-al87pro-bk3632) to deliver low-latency 2.4GHz GFSK wireless communication.

---

## Provenance & Clean-Room Statement / 独立实现声明

> **English**:
> The USB HID descriptors, 2.4GHz RF communication state machine, and packet parsers in this repository are independently implemented based on USB 2.0 / HID 1.11 specifications and publicly available BK2535 hardware register definitions. This project does not contain, link against, or distribute any vendor proprietary precompiled binary libraries, and is built entirely using the open-source **SDCC** (Small Device C Compiler) toolchain.
>
> **中文**:
> 本仓库中的 USB HID 描述符、2.4GHz 射频通信状态机及数据包解析逻辑均为基于 USB 2.0 / HID 1.11 规范及公开的 BK2535 硬件寄存器定义独立实现（Clean-Room Implementation）。本项目不包含、不链接任何厂商专有的预编译闭源静态库（`.lib`），完全由开源的 **SDCC** 编译器构建。

---

## Technical Specifications & Features

| Feature | Specification | Technical Details |
| :--- | :--- | :--- |
| **USB Polling Rate** | **1000Hz (1ms)** | Configured with `bInterval = 1` on USB Endpoint 1 for minimal input latency |
| **NKRO Format** | **15-byte Compact NKRO** | 1 Modifier byte + 14 bitmap bytes covering keys `0x04` to `0x73` (112 keys) |
| **Media Keys** | **Consumer Control (2-byte)** | Volume Up/Down, Mute, Play/Pause, Next/Prev Track, Calculator |
| **RF Protocol** | **2.4GHz GFSK (1Mbps/2Mbps)** | 16-channel dynamic frequency hopping across 4 channel groups |
| **LED Synchronization** | **Bidirectional ACK** | CapsLock / NumLock LED states piggybacked on RF ACK payloads to keyboard |
| **Pairing & Binding** | **Hardware Auto-Pairing** | Public pairing pipe address (`RX_Pair_Address`), auto-persisted to Flash NVR |

### Compatibility & Bug Fixes Over Stock Firmware
1. **Windows Error Code 10 (`0xC00000B9`) Fix**: Resolved HID report descriptor byte alignment issues that caused Windows Device Manager to fail enumeration with code 10.
2. **Android OTG Compatibility**: Configured standard `SubClass = 0` and `Protocol = 0` to prevent mobile operating systems from freezing during boot protocol negotiation.
3. **UART Debug Elimination**: Completely disabled blocking 9600-baud UART logging in the main loop (`ENABLE_UART_DEBUG=0`), eliminating USB packet drops and timing jitter.

---

## Building the Firmware

### Prerequisites
- **SDCC (Small Device C Compiler)** (version 4.2+ recommended)
  - On Ubuntu / Debian: `sudo apt install sdcc`
  - On Arch Linux: `sudo pacman -S sdcc`
  - On macOS: `brew install sdcc`
  - On Windows: Download from [SourceForge SDCC](https://sdcc.sourceforge.net/)
- Optional: `wine` (for running the Beken CRC generator `bin_to_crcbin.exe` on Linux)

### One-Click Build
Run the build script:

```bash
chmod +x build_dongle.sh
./build_dongle.sh
```

### Build Artifacts
Outputs are generated in `output/`:
* `dongle_bk2535.bin`: Raw 32KB binary image (padded with `0xFF`).
* `dongle_bk2535_crc.bin`: Official Beken-signed binary with 32-bit CRC appended at offset `0x7FFC`.

---

## Flashing Instructions

Flash `output/dongle_bk2535_crc.bin` to the BK2535 Dongle using your preferred hardware programmer (e.g. Beken Writer, Sinowealth ISP/ICP, or SPI programmer).

---

## Disclaimer & Legal Notice / 免责与知识产权声明

### English
1. **Purpose**: Provided solely for personal study, device maintenance (Right to Repair), and non-commercial interoperability research.
2. **Trademarks**: BK2535 and Beken are trademarks of Beken Corporation (博通集成). The maintainer claims copyright solely over their independent software implementation under the MIT license.
3. **No Warranty**: Provided "as is", without warranty of any kind. Use at your own risk.
4. **Takedown Policy**: If any copyright holder believes specific content should not be publicly shared, please contact the maintainer or open an Issue for prompt good-faith resolution.

### 中文
1. **非商业用途**：本项目仅供个人技术研究、设备自主维护（Right to Repair）及互操作性交流使用，严禁用于任何商业牟利。
2. **知识产权**：BK2535 芯片型号及硬件标识归博通集成所有。维护者依据 MIT 许可证对自研软件代码享有署名权。
3. **免责保证**：本项目按现状提供，不提供任何明示或暗示的保证，使用风险自负。
4. **善意合规**：如任何权利人有合规异议，请通过 Issue 联系，我们将以最快速度核实并妥善处理。
