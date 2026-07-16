# 02 — Vitis Platform, BSP, and UART Bridge Application

**Tool:** Vitis 2025.2  
**Vivado 2025 differences:** Minor — BSP `stdin`/`stdout` must be set in two separate locations and regenerated both times; easy to miss one.

---

## Overview

Create a Vitis platform from the exported `.xsa`, configure the BSP so UART1 handles terminal I/O, write a `main.c` polling bridge that forwards characters bidirectionally between UART0 (Pmod BLE) and UART1 (Tera Term), and run it on the ZedBoard.

---

## Key Concepts

- **Platform:** Vitis project layer that wraps the `.xsa` hardware description and generates BSP drivers.
- **BSP (Board Support Package):** Auto-generated C drivers for all peripherals defined in the hardware. Provides `XUartPs`, `xil_printf`, etc.
- **`ps7_uart_0`:** UART0 — connected to MIO 10/11 → JE → Pmod BLE.
- **`ps7_uart_1`:** UART1 — connected to MIO 48/49 → USB-UART on J14 → Tera Term.
- **`standalone_stdin` / `standalone_stdout`:** BSP setting that routes `xil_printf` and `scanf` to a specific UART. Must be `ps7_uart_1` (Tera Term) so boot messages appear in the terminal.
- **Polling bridge:** Continuously checks both UART RX buffers in a loop and forwards any received byte to the other UART. No interrupts — simple and sufficient at 115200 baud for this use case.
- **FSBL (First Stage Boot Loader):** Initializes PS at boot. Must be set to JTAG mode for this project.

---

## Vivado 2025 Differences

> **`stdin`/`stdout` must be set in two locations.** Under `zynq_fsbl → standalone` AND under `standalone_ps7_cortexa9_0 → standalone`. Regenerate BSP after each change. If you only set one, `xil_printf` output may not appear in Tera Term.

---

## Steps

### Platform Creation

1. Open Vitis. Set workspace to the `ws` folder inside `TeraTerm_ZedBoard_PmodBLE/`.

2. **File → New Component → Platform**.
   - Name: `ZedBLEplatform` (or your choice).
   - Under **Flow**, select: **Hardware design**.
   - Click **Browse** and select the `.xsa` file from the `vivado/` folder.
   - OS and Processor: leave defaults (standalone, ps7_cortexa9_0).
   - Click **Finish**.

### BSP Configuration

> **Critical — do this before building.** Failing to set both locations means `xil_printf` goes to UART0 (the BLE side) and nothing appears in Tera Term.

3. Hover over the platform → click the **gear icon** → platform settings dropdown opens.

4. Navigate to **`zynq_fsbl` → `standalone`**.
   - Set `standalone_stdin` → `ps7_uart_1`
   - Set `standalone_stdout` → `ps7_uart_1`

5. Navigate to **`zynq_fsbl` → `Board Support Package`** → click **Regenerate BSP**.

6. Navigate to **`standalone_ps7_cortexa9_0` → `standalone`**.
   - Set `standalone_stdin` → `ps7_uart_1`
   - Set `standalone_stdout` → `ps7_uart_1`

7. Navigate to **`standalone_ps7_cortexa9_0` → `Board Support Package`** → click **Regenerate BSP**.

8. **Build the Platform** (hammer icon or right-click platform → Build).

### Application Creation

9. **File → New Component → Application**.
   - Name: `ZedBLEapplication`
   - Hardware: select `ZedBLEplatform`
   - Leave all other settings as default.
   - Click **Finish**.

10. Build the application (hammer icon).

### main.c

11. In the Explorer: right-click **`ZedBLEapplication` → Sources → `src`** → **New File** → name it `main.c` → **OK**.

12. Copy-paste the following code:

```c
/*
 * Name: Ava Kirkland
 * Date: 6/11/2026
 * Company: Nspired Engineering
 * Description: Bidirectional UART bridge with polling.
 *
 * Prerequisites:
 *   - ZedBoard in JTAG boot mode (J7-J11 GND)
 *   - Pmod BLE connected to JE via jumper wires (TX/RX crossed)
 *   - Tera Term connected to ZedBoard UART COM port
 *   - Tera Term baud rate: 115200
 */

#include <stdio.h>
#include <xstatus.h>
#include <xuartps_hw.h>
#include "xuartps.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"

// UART instance structs
XUartPs Uart0;  // UART0 → Pmod BLE (MIO 10/11, JE)
XUartPs Uart1;  // UART1 → Tera Term (MIO 48/49, J14)

/*
 * Flush both UART RX FIFOs to discard stale bytes.
 * Call before starting the bridge loop if needed.
 */
void clear_uart() {
    u8 garbage;

    while (XUartPs_IsReceiveData(Uart0.Config.BaseAddress)) {
        XUartPs_Recv(&Uart0, &garbage, 1);
    }
    while (XUartPs_IsReceiveData(Uart1.Config.BaseAddress)) {
        XUartPs_Recv(&Uart1, &garbage, 1);
    }

    msleep(1);  // 1ms quiet period after flush
}

/*
 * Initialize both UARTs at 115200 baud.
 * Returns XST_SUCCESS or XST_FAILURE.
 */
int init_uart() {
    XUartPs_Config *cfg;
    int status;

    // --- UART1: Terminal (Tera Term) ---
    cfg = XUartPs_LookupConfig(XPAR_XUARTPS_1_BASEADDR);
    if (cfg == NULL) {
        xil_printf("UART1 LookupConfig FAILED\r\n");
        return XST_FAILURE;
    }

    status = XUartPs_CfgInitialize(&Uart1, cfg, cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        xil_printf("UART1 CfgInitialize FAILED\r\n");
        return status;
    }

    XUartPs_SetBaudRate(&Uart1, 115200);
    xil_printf("UART1 initialized (Terminal UART) @115200\r\n");

    // --- UART0: Pmod BLE ---
    cfg = XUartPs_LookupConfig(XPAR_XUARTPS_0_BASEADDR);
    if (cfg == NULL) {
        xil_printf("UART0 LookupConfig FAILED\r\n");
        return XST_FAILURE;
    }

    status = XUartPs_CfgInitialize(&Uart0, cfg, cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        xil_printf("UART0 CfgInitialize FAILED\r\n");
        return status;
    }

    XUartPs_SetBaudRate(&Uart0, 115200);
    xil_printf("UART0 initialized (BLE UART) @115200\r\n");

    xil_printf("Both UARTs ready.\r\n\r\n");
    return XST_SUCCESS;
}

int main() {
    u8 c;
    u8 newline[2] = {'\r', '\n'};
    u32 received;

    int status = init_uart();
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: UART init failed\r\n");
        return XST_FAILURE;
    }

    xil_printf("UART Bridge Running...\r\n");

    while (1) {

        // --- Terminal → BLE ---
        if (XUartPs_IsReceiveData(Uart1.Config.BaseAddress)) {
            received = XUartPs_Recv(&Uart1, &c, 1);
            if (received == 1) {
                if (c == '\r') {
                    // Echo newline to terminal on Enter
                    XUartPs_Send(&Uart1, newline, 2);
                } else {
                    // Echo character back to terminal
                    XUartPs_Send(&Uart1, &c, 1);
                }
                // Forward all characters to BLE regardless
                XUartPs_Send(&Uart0, &c, 1);
            }
        }

        // --- BLE → Terminal ---
        if (XUartPs_IsReceiveData(Uart0.Config.BaseAddress)) {
            received = XUartPs_Recv(&Uart0, &c, 1);
            if (received == 1) {
                XUartPs_Send(&Uart1, &c, 1);
            }
        }
    }
}
```

### Build and Run

> **Before running:** Complete hardware setup ([docs/03_hardware_setup.md](03_hardware_setup.md)) and open Tera Term connected to the ZedBoard UART COM port at 115200 baud. The application prints startup messages immediately on launch — if Tera Term is not open and connected first, you will miss them.

13. Build the platform first, then build the application (hammer icon on each).

14. Run the application via Vitis debug/run (green play button). The application is programmed to the board over JTAG.

---

## Debug Notes

- If Tera Term shows nothing after run: verify `standalone_stdin`/`standalone_stdout` are set to `ps7_uart_1` in both BSP locations and BSP was regenerated. See [bugs_and_fixes.md](bugs_and_fixes.md) Bug #1.
- The `clear_uart()` function is defined but commented out in the main loop. Enable it if stale bytes appear at startup.
- The `while (XUartPs_IsTransmitFull(...))` guard lines are commented out. They are safe to enable if TX FIFO overflow is suspected at higher throughput.

---

## Expected Terminal Output on Startup

```
UART1 initialized (Terminal UART) @115200
UART0 initialized (BLE UART) @115200
Both UARTs ready.

UART Bridge Running...
```

After this, anything typed in Tera Term is echoed locally and forwarded to Pmod BLE. Pmod BLE responses appear in Tera Term.

---

## Observed Results / Screenshots

![Create Platform Summary](images/Screenshot%202026-06-11%20160805.png)
![UART1 BSP stdin/stdout zynq_fsbl](images/Screenshot%202026-06-11%20161215.png)
![Regenerate BSP zynq_fsbl](images/Screenshot%202026-06-11%20161315.png)
![UART1 BSP standalone_ps7_cortexa9_0](images/Screenshot%202026-06-11%20161502.png)
![Build Platform](images/Screenshot%202026-06-11%20161704.png)
![Application Platform Selection](images/Screenshot%202026-06-11%20161844.png)
![Application Summary](images/Screenshot%202026-06-11%20161919.png)
![Create main.c](images/Screenshot%202026-06-11%20162053.png)
