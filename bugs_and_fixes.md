# Bugs and Fixes

All bugs encountered across the project, in roughly chronological order.

---

## Bug #1 — No output in Tera Term after Vitis application runs

**Symptom:** Vitis reports the application launched successfully. Tera Term shows nothing — not even the `UART1 initialized...` startup message.

**Root cause:** `standalone_stdin` and `standalone_stdout` were set to `ps7_uart_1` in only one BSP location, not both. Vitis 2025.2 requires this setting in two separate places:
- `zynq_fsbl → standalone`
- `standalone_ps7_cortexa9_0 → standalone`

If either is left pointing to `ps7_uart_0` (the default), `xil_printf` output goes to UART0 (the BLE side) and nothing appears in Tera Term.

**Fix:**
1. Open platform settings (gear icon on the platform).
2. Set `standalone_stdin` and `standalone_stdout` to `ps7_uart_1` under `zynq_fsbl → standalone`. Regenerate BSP.
3. Set `standalone_stdin` and `standalone_stdout` to `ps7_uart_1` under `standalone_ps7_cortexa9_0 → standalone`. Regenerate BSP.
4. Rebuild the platform, then rebuild and rerun the application.

**Lesson:** In Vitis 2025.2, the BSP stdin/stdout setting has two independent locations. Set and regenerate both. Checking one is not sufficient.

---

## Bug #2 — Pmod BLE does not respond to `$$$` (no `CMD>`)

**Symptom:** Typing `$$$` in Tera Term produces no response from Pmod BLE. Terminal appears to accept input but nothing comes back.

**Root cause (most likely):** TX and RX wires are not crossed. Plugging Pmod BLE directly into the JE connector, or wiring TXD→TXD and RXD→RXD, means both devices are transmitting to each other's transmitters. Neither receives anything.

**Root cause (alternate):** Baud rate mismatch. Pmod BLE defaults to 115200. If Tera Term or the USB-TTL converter is set to a different baud rate, the characters arrive corrupted.

**Fix:**
- Verify the wiring cross: Pmod BLE pin 2 (RX) → JE3/MIO11 (TX), Pmod BLE pin 3 (TX) → JE2/MIO10 (RX).
- Confirm Tera Term baud rate is 115200.
- If using USB-TTL: confirm converter is also 115200 (separate from the 9600 used in loopback test).

**Lesson:** UART TX/RX cross is one of the most common wiring mistakes. Verify with a table before powering on, not after.

---

## Bug #3 — nRF Connect connects to Pmod BLE but no writable characteristic appears

**Symptom:** nRF Connect (Android or iOS) connects to the RN4871 successfully. The phone shows services but there is no Unknown Characteristic with Write/Notify/Indicate properties. No BLE data channel is available.

**Root cause:** UART Transparent Service was not enabled. The RN4871 ships with default services that do not include the UART Transparent profile. `SS,C0` must be issued in command mode and the module must be rebooted before the characteristic appears.

**Fix:**
```
$$$
SS,C0
R,1
```
After reboot, verify with `$$$` → `D` → confirm `Services=C0`.

**Lesson:** Always verify `Services=C0` with the `D` command before attempting a phone connection. Connecting without this step wastes time debugging the app side when the issue is module configuration.

---

## Bug #4 — First `$$$` after a Tera Term session requires pressing Enter first

**Symptom:** On the first attempt to enter command mode (`$$$`) in a new Tera Term session — particularly after reconnecting — there is no `CMD>` response. Pressing Enter first, then typing `$$$` works.

**Root cause:** The RN4871 may be in an intermediate state (e.g., coming out of a previous data mode session or a reconnect). A carriage return clears the buffer and returns the module to a receptive state.

**Fix:** Press Enter once before typing `$$$` in a new or reconnected session.

**Lesson:** When `$$$` doesn't work immediately, a bare Enter is the first thing to try. This is a known RN4871 quirk, not a wiring or config issue.

---

## Bug #5 — Tera Term shows garbage characters instead of readable output

**Symptom:** Tera Term displays garbled text, question marks, or seemingly random symbols instead of the expected `UART1 initialized...` messages or BLE responses.

**Root cause:** Baud rate mismatch between the sender and Tera Term. UART is extremely sensitive to baud rate — even small mismatches produce corrupted output.

**Fix:** Confirm both sides are 115200:
- Tera Term: Setup → Serial Port → Speed: 115200
- Vitis `main.c`: `XUartPs_SetBaudRate(&Uart1, 115200)` — already set in the provided code.
- USB-TTL converter: Tera Term setting must match the baud the converter is operating at.

**Lesson:** Garbled UART output is almost always a baud rate mismatch. Check this before any other debugging.
