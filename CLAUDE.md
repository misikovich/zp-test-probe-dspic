# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Hardware tester for an EV charger (servicing tool). Tests: FPGA, RCD, relays, voltages, eMeters, lock motor, comms.

**Target MCU:** PIC24EP256MC206 (16-bit dsPIC, 256 KB flash, 16 KB SRAM)  
**Toolchain:** XC16 v2.10 — C89/C90 base + select C99 extensions (`<stdint.h>`, `static inline`). No VLAs. `//` line comments are always illegal — XC16 compiles C files in C89/C90 mode unconditionally; use `/* */`.  
**RTOS:** FreeRTOS (PIC24/dsPIC port, Timer1 tick, heap_4 allocator)  
**Clock:** Safe default is FRC oscillator: Fosc = 7.37 MHz, Fcy = 3.685 MHz. `src/project_clock.h` can enable FRC+PLL for Fosc ~= 140.03 MHz, Fcy ~= 70.015 MHz.

## Build

**From VS Code:** `Ctrl+Shift+B` — MPLAB extension drives cmake + ninja.  
Two configs: **default** (debug, `-g -O0`) and **default.production** (production).

**CLI ninja build:**
```powershell
$ninja = "C:\Users\Apollo Prog\.mplab\app-finder\apps\ninja\v1.13.2\ninja.exe"
& $ninja -C "C:\Users\Apollo Prog\MPLABProjects\pwr-probe-test\_build\pwr-probe-test\default"
```

**CLI single-file compile check:**
```
"c:\Program Files\Microchip\xc16\v2.10\bin\xc16-gcc.exe" \
  -DXPRJ_default=default -D__DEBUG \
  -D__PIC24EP256MC206__ -D__24EP256MC206__ -D__PIC24E__ \
  -x c -g -mcpu=24EP256MC206 -O0 -msmart-io=1 -Wall -msfr-warn=off \
  "-mdfp=C:/Users/Apollo Prog/.mchp_packs/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16" \
  -c src/main.c
```
No output = clean. Errors go to stderr.

Output artifacts: `out/pwr-probe-test/default.elf`, `out/pwr-probe-test/default.hex`

## Flash

**PICkit 4** (target must be powered, or add `-W` for PICkit-supplied 3.3 V):
```
.\flash.ps1
```
`flash.ps1` calls `ipecmd.exe -TPPK4 -P24EP256MC206 -F<hex> -M -OL`

## Simulate

MDB (MPLAB Debugger) can run the ELF in software simulation — no hardware needed:
```powershell
& "C:\Program Files\Microchip\MPLABX\v6.20\mplab_platform\bin\mdb.bat" script.mdbscript
```
Script syntax: `Device PIC24EP256MC206` / `Hwtool SIM` / `Program "path.elf"` / `Run` / `Wait 5000` / `Halt` / `Print LATB` / `Quit`

## Architecture

### Source layout
- `src/` — application code (`main.c` entry point)
- `src/st7789v.c/.h` — project-facing ST7789V display API and PIC24 SPI/GPIO backend
- `FreeRTOS/Source/` — kernel (do not modify except portmacro.h and port.c)
- `FreeRTOSConfig.h` — at project root; kernel tuning for this device
- `FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/` — PIC24E port (`portasm_PIC24.S`, `port.c`, `portmacro.h`)

### `src/amazing_utils.h` — shared utilities
Single utility header included by all source files. Defines:
- **Type aliases:** `u8`, `u16`, `u32` (mapped to `uint8/16/32_t`)
- **Macros:** `forever` (`for(;;)`), `unused(v)` (`(void)(v)`), `vTaskDelayMS(ms)` (`vTaskDelay(pdMS_TO_TICKS(ms))`)
- **Pin abstraction** (see below)

### Pin abstraction (`src/`)
`Pin` struct holds pointers to the actual LAT/TRIS/ANSEL registers + a bitmask — not copies of values. All GPIO ops go through `pin_high()`, `pin_low()`, `pin_toggle()`, `pin_output()`, `pin_set()`, `pin_ansel()`. ANSEL field is `NULL` for digital-only pins (safe no-op).

### FreeRTOS configuration
- Timer1 tick at 1 ms (`configTICK_RATE_HZ = 1000`, PR1 = 460 for default Fcy/8)
- `configTICK_TYPE_WIDTH_IN_BITS = 1` → 32-bit tick (not `configUSE_16_BIT_TICKS`)
- Heap: 6 KB (`heap_4`), stack headroom ~22 KB
- `configKERNEL_INTERRUPT_PRIORITY = 1` — T1IP matches this value in port.c
- `configASSERT(x)`: disables interrupts + spins forever — a halted MCU means an assert fired
- **Disabled** (do not include headers for these without enabling in FreeRTOSConfig.h): mutexes, recursive mutexes, counting semaphores, co-routines, software timers, static allocation, queue sets, trace facility, stack overflow check

### cmake customisation — user.cmake
The MPLAB extension regenerates `cmake/pwr-probe-test/default/CMakeLists.txt` and `cmake/pwr-probe-test/default.production/CMakeLists.txt` but preserves `user.cmake` alongside each. **Both** `user.cmake` files must be kept identical — they add device macros, XC16 include dirs, FreeRTOS paths, display sources, and `-mno-eds-warn` to the compile targets. If MPLAB regenerates cmake and includes disappear, check these files.

### IntelliSense
- `xc.h` resolves via `includePath` in `.vscode/c_cpp_properties.json` (cpptools)
- Device macros explicit in `defines` — XC16 injects them from `-mcpu` but cpptools does not
- Do NOT add `"C_Cpp.default.configurationProvider": "microchip.mplab-clangd"` to `.vscode/settings.json` — MPLAB clangd extension overrides include paths with empty config until its binary downloads, breaking IntelliSense

## Key Files

| Path | Purpose |
|------|---------|
| `.vscode/pwr-probe-test.mplab.json` | MPLAB project config — do not delete; uses `**/*` glob to pick up all source files |
| `cmake/pwr-probe-test/default/user.cmake` | Build customisation — debug config |
| `cmake/pwr-probe-test/default.production/user.cmake` | Build customisation — production config (keep in sync with debug) |
| `src/st7789v.c/.h` | ST7789V display API and PIC24 SPI/GPIO backend |
| `FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/portmacro.h` | Patched: adds `#include <xc.h>` for `SET_CPU_IPL`; guards `SIZE_MAX` |
| `FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/port.c` | Patched: adds `#include <xc.h>` + `<libpic30.h>` for SFR access |
| `out/` | Build artifacts (ELF, HEX) |
| `_build/`, `cmake/` | CMake build tree — deletable (user.cmake survives; regenerated files do not) |
