# Tool Version Differences

Differences between Vivado 2025.2.1 / Vitis 2025.2 behavior and what tutorials or older documentation describe. Observed on Windows.

---

## 1. BSP `stdin`/`stdout` must be set in two separate locations in Vitis 2025.2

**Tutorial/docs behavior:** Set `standalone_stdin` and `standalone_stdout` once in the BSP settings.

**2025.x behavior:** Two independent BSP entries exist and must both be updated:
- `zynq_fsbl → standalone`
- `standalone_ps7_cortexa9_0 → standalone`

BSP must be regenerated after each change.

**Impact:** If only one location is updated, `xil_printf` output routes to the wrong UART. Nothing appears in Tera Term.

**Fix:** Set both to `ps7_uart_1`, regenerate BSP in both locations. **Confirmed working.**

---

## 2. Vivado synthesis may fail to launch via GUI button — `[Common 17-180]` error

**Tutorial/docs behavior:** Click "Run Synthesis" in Flow Navigator. Synthesis launches.

**2025.x behavior:** On some Windows configurations, the GUI button produces:  
`[Common 17-180] Spawn failed: The operation completed successfully`  
and synthesis does not start despite the success message.

**Impact:** Blocks synthesis unless workaround is used.

**Fix:** Use the Tcl Console instead of the GUI button:
```tcl
reset_run synth_1
launch_runs synth_1 -jobs 4
```
**Confirmed working.** (GUI button is a known Vivado 2025 Windows bug — not encountered in this project but documented here for reference.)

---

## 3. RN4871 ships with old firmware — BLE features may be incomplete below v1.44

**Tutorial/docs behavior:** Tutorials assume a recent firmware version.

**2025.x / current hardware behavior:** Pmod BLE units may ship with firmware as old as v1.18.3. Some BLE commands and behaviors documented in the RN4870/71 User Guide may not work correctly or at all on old firmware.

**Impact:** BLE connection or `SS,C0` UART Transparent Service may not function as expected.

**Fix:** Update to v1.44 using `isupdate.exe` and the `.H##` hex files from Microchip. See [docs/04_usb_ttl_and_firmware.md](04_usb_ttl_and_firmware.md). **Confirmed working.**

---

## 4. Vitis 2025.2 TCL driver loading for custom IP is broken

**Tutorial/docs behavior:** BSP auto-discovers and loads custom IP drivers.

**2025.x behavior:** Custom IP driver files are not automatically loaded by the BSP in Vitis 2025.x.

**Impact:** Custom IP drivers are unavailable at compile time unless manually copied.

**Fix (temporary workaround):** Copy custom driver files into `src/` alongside `main.c` manually.  
**Note:** This does not apply to this project (no custom IP used), but documented here as a known Vitis 2025.2 issue for future reference.
