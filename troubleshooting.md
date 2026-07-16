# Troubleshooting — Quick Reference

Symptom → most likely cause → fix. Ordered from most to least common within each section.

---

## Tera Term / UART Output

### Nothing appears in Tera Term after running the Vitis application

1. **BSP stdin/stdout not set correctly.**  
   Set `standalone_stdin` and `standalone_stdout` to `ps7_uart_1` in both `zynq_fsbl → standalone` AND `standalone_ps7_cortexa9_0 → standalone`. Regenerate BSP in both. Rebuild platform. Rebuild and rerun application.  
   → See [bugs_and_fixes.md Bug #1](bugs_and_fixes.md#bug-1--no-output-in-tera-term-after-vitis-application-runs)

2. **Wrong COM port selected in Tera Term.**  
   Open Device Manager → Ports (COM & LPT). Identify the ZedBoard USB-UART port (usually labeled "Silicon Labs" or similar). Select that COM port in Tera Term.

3. **Tera Term baud rate is not 115200.**  
   Setup → Serial Port → Speed: 115200. Restart after changing.

4. **Application did not deploy to board.**  
   Confirm Vitis run/debug completed without errors. Check that JTAG boot jumpers (J7–J11) are set to GND.

---

### Garbled / garbage characters in Tera Term

1. **Baud rate mismatch.**  
   Both sides must be 115200. Check Tera Term serial port settings and confirm `XUartPs_SetBaudRate` in `main.c` matches.  
   → See [bugs_and_fixes.md Bug #5](bugs_and_fixes.md#bug-5--tera-term-shows-garbage-characters-instead-of-readable-output)

---

## Pmod BLE / UART0 Communication

### `$$$` typed in Tera Term — no `CMD>` response

1. **TX/RX wires not crossed.**  
   Verify: Pmod BLE pin 2 (RX) → JE3 (MIO11, TX). Pmod BLE pin 3 (TX) → JE2 (MIO10, RX).  
   → See [bugs_and_fixes.md Bug #2](bugs_and_fixes.md#bug-2--pmod-ble-does-not-respond-to--no-cmd)

2. **Baud rate mismatch on UART0.**  
   Pmod BLE defaults to 115200. Confirm `XUartPs_SetBaudRate(&Uart0, 115200)` in `main.c`.

3. **Reconnecting in same Tera Term session — press Enter first.**  
   Press Enter once before typing `$$$`. Known RN4871 behavior.  
   → See [bugs_and_fixes.md Bug #4](bugs_and_fixes.md#bug-4--first--after-a-tera-term-session-requires-pressing-enter-first)

4. **Pmod BLE not powered.**  
   Blue LED should blink on power-up. Check VCC and GND connections to JE.

---

### Pmod BLE blue LED not blinking

1. **No power.**  
   Check VCC connection (Pmod BLE pin 6 or 12 → JE6 or JE12). Verify ZedBoard is powered on (SW8).

2. **GND not connected.**  
   Verify GND connection (Pmod BLE pin 5 or 11 → JE5 or JE11).

---

## BLE / Phone Connection

### nRF Connect connects but no writable characteristic appears

1. **`SS,C0` not applied or module not rebooted after.**  
   In command mode: `SS,C0` → `R,1`. Verify with `$$$` → `D` → confirm `Services=C0`.  
   → See [bugs_and_fixes.md Bug #3](bugs_and_fixes.md#bug-3--nrf-connect-connects-to-pmod-ble-but-no-writable-characteristic-appears)

---

### Data not flowing from phone to Tera Term after connection

1. **Indications not enabled in nRF Connect.**  
   Tap the two-arrows icon on the Unknown Characteristic. It should show an X (Android) or turn blue (iOS).

2. **Module not in data mode.**  
   If `---` was not typed after command mode setup, the module may still be in command mode. Type `---` to exit to data mode.

---

### Data not flowing from Tera Term to phone

1. **Two-arrows icon not enabled.**  
   Phone must subscribe to indications/notifications before it can receive data. See above.

2. **Data mode not active.**  
   Verify module is not in command mode (`CMD>` prompt). Type `---` to exit command mode if present.

---

## USB-to-TTL Converter

### Tera Term loopback test shows nothing (Procedure 1 in doc 04)

1. **TX/RX short not installed on converter.**  
   Ensure the short jumper is bridging TXD and RXD pins on the converter.

2. **Wrong baud rate.**  
   Loopback test uses 9600, not 115200. Confirm Tera Term is set to 9600 for this test.

3. **Local echo is ON in Tera Term.**  
   Setup → Terminal → Local Echo: OFF. With echo ON, characters appear regardless of loopback — this masks a failed test.

4. **Wrong COM port.**  
   Check Device Manager → Ports (COM & LPT) for the converter's assigned COM port.

---

### Pmod BLE does not respond when connected via USB-TTL converter

1. **Converter voltage not set to 3.3V.**  
   Unplug converter, set voltage jumper to 3.3V, rewire and reconnect.

2. **TX/RX not correctly connected.**  
   Converter TXD → Pmod BLE RX (pin 2). Converter RXD → Pmod BLE TX (pin 3).

3. **Baud rate mismatch.**  
   Tera Term must be at 115200 for Pmod BLE (different from 9600 used in loopback test).

---

## Build and Synthesis Errors

### Vivado synthesis fails to launch — `[Common 17-180] Spawn failed`

Known Vivado 2025 Windows bug. Use Tcl Console:
```tcl
reset_run synth_1
launch_runs synth_1 -jobs 4
```
→ See [tool_version_differences.md](tool_version_differences.md#2-vivado-synthesis-may-fail-to-launch-via-gui-button--common-17-180-error)

---

## Nuclear Options

If targeted fixes don't resolve a Vitis issue (platform/BSP in a broken state):

1. **Rebuild platform from scratch:** Delete the platform component in Vitis, re-create from the `.xsa` file, reconfigure BSP stdin/stdout in both locations, regenerate BSP, rebuild.

2. **New Vitis workspace:** Create a new `ws/` directory, recreate platform and application from scratch. Sometimes the workspace itself gets corrupted.

3. **Re-export hardware from Vivado:** If the `.xsa` file may be stale or corrupted, re-run Export Hardware from Vivado (File → Export → Export Hardware → Include Bitstream) and recreate the Vitis platform from the new `.xsa`.
