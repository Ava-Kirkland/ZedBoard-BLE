# Tera Term ↔ ZedBoard ↔ Pmod BLE

A UART bridge between a PC terminal (Tera Term) and a Pmod BLE (RN4871) module via the ZedBoard's PS UART, with additional support for direct USB-TTL testing, firmware updates, and BLE communication with Android and iOS devices.

---

## Table of Contents

| File | Description | Status |
|------|-------------|--------|
| [docs/01_vivado_block_design.md](docs/01_vivado_block_design.md) | Vivado project setup and ZYNQ PS UART configuration | ✅ |
| [docs/02_vitis_uart_bridge.md](docs/02_vitis_uart_bridge.md) | Vitis platform, BSP, application, and `main.c` UART bridge code | ✅ |
| [docs/03_hardware_setup.md](docs/03_hardware_setup.md) | Physical wiring, JE pinout, boot jumpers | ✅ |
| [docs/04_usb_ttl_and_firmware.md](docs/04_usb_ttl_and_firmware.md) | USB-to-TTL converter testing and RN4871 firmware update to v1.44 | ✅ |
| [docs/05_ble_android.md](docs/05_ble_android.md) | Connecting Pmod BLE to Android via nRF Connect | ✅ |
| [docs/06_ble_ios.md](docs/06_ble_ios.md) | Connecting Pmod BLE to iOS via nRF Connect | ✅ |
| [docs/bugs_and_fixes.md](docs/bugs_and_fixes.md) | All bugs encountered: symptom, root cause, fix, lesson | ✅ |
| [docs/tool_version_differences.md](docs/tool_version_differences.md) | Vivado/Vitis 2025.x behavior vs. tutorial/docs | ✅ |
| [docs/troubleshooting.md](docs/troubleshooting.md) | Quick-reference symptom → cause → fix lookup | ✅ |

---

## Hardware

| Component | Details |
|-----------|---------|
| Board | ZedBoard Zynq Evaluation and Development Kit |
| BLE Module | Pmod BLE — Digilent (RN4871 chip) |
| USB-UART Converter | DSD TECH SH-U09C5 (~$15, FTDI-based) |
| Pmod Connector | **JE only** (PS-connected; JA–JD are PL only) |
| Firmware Version | RN4871 v1.44 (06/21/2024) |

## Software & Tools

| Tool | Version |
|------|---------|
| Vivado | 2025.2.1 |
| Vitis | 2025.2 |
| Tera Term | Latest |
| nRF Connect for Mobile | Latest (Android & iOS) |

---

## System Architecture

```
┌────────────────────────────────────────────────────────────┐
│                          ZedBoard                          │
│                                                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │               ZYNQ PS (ARM Cortex-A9)                │  │
│  │                                                      │  │
│  │  UART1 (MIO 48/49) ◄─────────────────────────────►  │  │
│  │  [Terminal / Tera Term]             USB-UART J14     │  │
│  │                                                      │  │
│  │  UART0 (MIO 10/11) ◄─────────────────────────────►  │  │
│  │  [Pmod BLE]                         JE Pin 2/3      │  │
│  │                                                      │  │
│  │        main.c: polling bridge loop                   │  │
│  │        UART1 RX → UART0 TX                          │  │
│  │        UART0 RX → UART1 TX                          │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                            │
└──────────────────────────┬─────────────────────────────────┘
                          │ JE Connector (PS MIO 10/11)
                          │ Female-to-Male jumper wires
                          │ TX/RX CROSSED
                          ▼
                ┌─────────────────┐
                │    Pmod BLE     │
                │    (RN4871)     │
                │                 │
                │    BLE 4.2      │
                └────────┬────────┘
                         │ Bluetooth Low Energy
                ┌────────┴────────┐
                │   nRF Connect   │
                │  Android / iOS  │
                └─────────────────┘

PC (Tera Term) ◄──► USB-UART (J14) ◄──► ZedBoard ◄──► Pmod BLE ◄──► Phone
```

---

## Repository Structure

```
TeraTerm_ZedBoard_PmodBLE/
├── README.md
├── vivado/                          ← Vivado project files
├── ws/                              ← Vitis workspace
│   └── ZedBLEapplication/
│       └── src/
│           └── main.c
└── docs/
    ├── 01_vivado_block_design.md
    ├── 02_vitis_uart_bridge.md
    ├── 03_hardware_setup.md
    ├── 04_usb_ttl_and_firmware.md
    ├── 05_ble_android.md
    ├── 06_ble_ios.md
    ├── bugs_and_fixes.md
    ├── tool_version_differences.md
    ├── troubleshooting.md
    └── images/
```

---

## ⚠️ Key Tips — Read Before You Start

These are the lessons that cost the most time. Read them first.

**1. JE is the only Pmod connector that works for this project.**
JA–JD are PL-connected. UART0 is hardwired to PS MIO 10/11, which only routes to JE. Using any other Pmod connector will not work. See [docs/01_vivado_block_design.md](docs/01_vivado_block_design.md).

**2. Do the USB-to-TTL converter test and firmware update before starting Vivado.**
The Pmod BLE ships with old firmware that can cause unreliable BLE behavior. Update to v1.44 or newer using only the USB-TTL converter and a PC — no ZedBoard required. Doing this first means the module is verified working before it goes anywhere near the full project. See [docs/04_usb_ttl_and_firmware.md](docs/04_usb_ttl_and_firmware.md).

**3. Cross TX/RX when wiring Pmod BLE to JE — do not plug in directly.**
Pmod BLE pin 2 is RX, Pmod BLE pin 3 is TX. JE pin 2 (MIO10) is UART0 RX, JE pin 3 (MIO11) is UART0 TX. Plugging Pmod BLE directly into JE connects TX→TX and RX→RX. Use jumper wires and cross them. See [docs/03_hardware_setup.md](docs/03_hardware_setup.md).

**4. Set the USB-to-TTL converter to 3.3V before wiring.**
Pmod BLE operates at 3.3V logic. Setting the converter to 5V while connected will damage the module. Set voltage *before* connecting any wires, with converter unplugged. See [docs/04_usb_ttl_and_firmware.md](docs/04_usb_ttl_and_firmware.md).

**5. Set `standalone_stdin`/`standalone_stdout` to `ps7_uart_1` in both BSP locations.**
This must be set under `zynq_fsbl → standalone` AND `standalone_ps7_cortexa9_0 → standalone`, then BSP regenerated in both. Missing either one causes Tera Term to receive no output. See [docs/02_vitis_uart_bridge.md](docs/02_vitis_uart_bridge.md).

**6. Update firmware to v1.44 or newer before using BLE features.**
Older firmware (e.g., v1.18.3) may have missing or broken BLE commands. Updating requires shorting J1 on the Pmod BLE to enter test/boot mode. See [docs/04_usb_ttl_and_firmware.md](docs/04_usb_ttl_and_firmware.md).

**7. Run `SS,C0` and reboot before attempting any phone connection.**
The UART Transparent Service (`0x40`) must be enabled alongside Device Info (`0x80`) before the BLE data channel appears in nRF Connect. Without this, the phone connects but there is no writable characteristic. See [docs/05_ble_android.md](docs/05_ble_android.md) and [docs/06_ble_ios.md](docs/06_ble_ios.md).

**8. The program is wiped from board memory on power-off.**
The application runs from RAM. Every power cycle requires re-running from Vitis. The board does not persist the application after SW8 is switched off.

**9. Tera Term baud rate must match — 115200.**
If Tera Term is set to any other baud rate, the UART bridge will produce garbage output or nothing. Set this before launching the Vitis application.

---

## External Resources

- [ZedBoard Hardware User Guide](https://files.digilent.com/resources/programmable-logic/zedboard/ZedBoard_HW_UG_v2_2.pdf)
- [Pmod BLE Reference Manual — Digilent](https://digilent.com/reference/pmod/pmodble/reference-manual?redirect=1)
- [RN4870/71 User Guide — Microchip](https://ww1.microchip.com/downloads/en/DeviceDoc/RN4870-71-Bluetooth-Low-Energy-Module-User-Guide-DS50002466C.pdf)
- [RN4871 Digilent User Guide](https://digilent.com/reference/_media/reference/pmod/pmodble/rn4871_user_guide.pdf)
- [RN4870/1 Documentation Page — Microchip](https://www.microchip.com/en-us/product/RN4870#Documentation)
- [XUartPs API Reference — Xilinx](https://xilinx.github.io/embeddedsw.github.io/uartps/doc/html/api/group__uartps.html)
- [DSD TECH SH-U09C5 USB-TTL Converter](https://www.deshide.com/product-details_SH-U09C5.html)
- [Firmware Update Tutorial — martyncurrey.com](https://www.martyncurrey.com/arduino-with-rn48701/)
- [ZedBoard + WiFi ESP8266 Tutorial (reference series)](https://www.youtube.com/watch?v=z-ULdLuhKAU&list=PLXHMvqUANAFOviU0J8HSp0E91lLJInzX1&index=75)
- [ZedBoard UART Data Transfer Tutorial (reference series)](https://www.youtube.com/watch?v=lzQ9hJ-wevg&list=PLXHMvqUANAFOviU0J8HSp0E91lLJInzX1&index=22)
