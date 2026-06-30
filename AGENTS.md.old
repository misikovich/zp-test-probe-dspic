# AGENTS.md

This file provides guidance to Codex when working with code in this repository.

## Project

Hardware tester for an EV charger servicing tool. Tests include FPGA, RCD,
relays, voltages, eMeters, lock motor, and comms.

**Target MCU:** PIC24EP256MC206 (16-bit dsPIC, 256 KB flash, 16 KB SRAM)  
**Toolchain:** XC16 v2.10. C files are compiled as C89/C90 with select C99
extensions such as `<stdint.h>` and `static inline`. Do not use VLAs. `//`
line comments are illegal in C files; use `/* */`.  
**RTOS:** FreeRTOS PIC24/dsPIC port, Timer1 tick, heap_4 allocator.  
**Clock:** Safe default is FRC oscillator, Fosc = 7.37 MHz, Fcy = 3.685 MHz.
`src/project_clock.h` can enable FRC+PLL for Fosc ~= 140.03 MHz, Fcy ~= 70.015 MHz.

## Build

From VS Code: `Ctrl+Shift+B`. The MPLAB extension drives CMake and Ninja.

Two configs:
- `default`: debug, `-g -O0`
- `default.production`: production

CLI debug build:
```powershell
$ninja = "C:\Users\Apollo Prog\.mplab\app-finder\apps\ninja\v1.13.2\ninja.exe"
& $ninja -C "C:\Users\Apollo Prog\MPLABProjects\pwr-probe-test\_build\pwr-probe-test\default"
```

CLI production build:
```powershell
$ninja = "C:\Users\Apollo Prog\.mplab\app-finder\apps\ninja\v1.13.2\ninja.exe"
& $ninja -C "C:\Users\Apollo Prog\MPLABProjects\pwr-probe-test\_build\pwr-probe-test\default.production"
```

CLI single-file compile check:
```powershell
& "c:\Program Files\Microchip\xc16\v2.10\bin\xc16-gcc.exe" `
  -DXPRJ_default=default -D__DEBUG `
  -D__PIC24EP256MC206__ -D__24EP256MC206__ -D__PIC24E__ `
  -x c -g -mcpu=24EP256MC206 -O0 -msmart-io=1 -Wall -msfr-warn=off `
  "-mdfp=C:/Users/Apollo Prog/.mchp_packs/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16" `
  -c src/main.c
```

No output means the compile check is clean. Errors go to stderr.

Output artifacts:
- `out/pwr-probe-test/default.elf`
- `out/pwr-probe-test/default.hex`
- `out/pwr-probe-test/production/default-production.elf`
- `out/pwr-probe-test/production/default-production.hex`

## Flash

PICkit 4. The target must be powered, or add `-W` for PICkit-supplied 3.3 V:
```powershell
.\flash.ps1
```

`flash.ps1` calls `ipecmd.exe -TPPK4 -P24EP256MC206 -F<hex> -M -OL`.

## Simulate

MDB can run the ELF in software simulation with no hardware:
```powershell
& "C:\Program Files\Microchip\MPLABX\v6.20\mplab_platform\bin\mdb.bat" script.mdbscript
```

Script syntax example:
`Device PIC24EP256MC206` / `Hwtool SIM` / `Program "path.elf"` / `Run` /
`Wait 5000` / `Halt` / `Print LATB` / `Quit`

## Architecture

### Source Layout

- `src/`: application code.
- `src/main.c`: config bits, hardware init, FreeRTOS tasks.
- `src/amazing_utils.h`: shared type aliases, task delay macro, GPIO helpers.
- `src/pwm.h`, `src/pwm.c`: simple PWM driver for LEDs and basic DC motors.
- `src/st7789v.h`, `src/st7789v.c`: project-facing ST7789V display API and PIC24 SPI/GPIO backend.
- `FreeRTOS/Source/`: kernel. Do not modify except local port patches.
- `FreeRTOSConfig.h`: kernel tuning for this device.
- `FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/`: PIC24E port.

### Shared Utilities

`src/amazing_utils.h` defines:
- Type aliases: `u8`, `u16`, `u32`.
- Macros: `forever`, `unused(v)`, `vTaskDelayMS(ms)`.
- `Pin` abstraction and helpers.

`Pin` stores pointers to LAT/TRIS/ANSEL registers plus a bitmask. These are
register pointers, not copied values. GPIO operations should go through
`pin_high()`, `pin_low()`, `pin_toggle()`, `pin_output()`, `pin_set()`, and
`pin_ansel()`. `ANSEL` may be `NULL` for digital-only pins; `pin_ansel()` then
becomes a safe no-op.

### PWM

`src/pwm.c` owns Timer2 and OC1-OC4. Timer1 is reserved for the FreeRTOS tick.
All PWM channels share one frequency and have independent duty registers.

Use `PwmPin` values with:
- a `Pin`
- an RP number
- an OC channel number from 1 to 4

Duty uses `0..PWM_DUTY_MAX`, where `PWM_DUTY_MAX` is `1000`.
- `0` disables OC/PPS and drives the pin low as GPIO.
- `PWM_DUTY_MAX` disables OC/PPS and drives the pin high as GPIO.
- Middle values map PPS and run edge-aligned PWM.

The driver is intentionally simple for LEDs and simple DC motor drivers. It is
not intended for FOC or complementary motor-control PWM.

Current LED mappings in `src/main.c`:
- `LED_R`: RB8, RP40, OC1
- `LED_G`: RB10, RP42, OC2
- `LED_B`: RB6, RP38, OC3
- `LED_W`: RB5, RP37, OC4

### FreeRTOS Configuration

- Timer1 tick at 1 ms: `configTICK_RATE_HZ = 1000`, PR1 = 460 for default Fcy/8.
- `configTICK_TYPE_WIDTH_IN_BITS = 1` means 32-bit tick.
- Heap is 6 KB via `heap_4`.
- Stack headroom is about 22 KB.
- `configKERNEL_INTERRUPT_PRIORITY = 1`; T1IP matches this value in `port.c`.
- `configASSERT(x)` disables interrupts and spins forever.
- Disabled: mutexes, recursive mutexes, counting semaphores, co-routines,
  software timers, static allocation, queue sets, trace facility, stack
  overflow check.

### CMake Customisation

The MPLAB extension regenerates:
- `cmake/pwr-probe-test/default/CMakeLists.txt`
- `cmake/pwr-probe-test/default.production/CMakeLists.txt`

It preserves `user.cmake` beside each config. Keep these two files identical:
- `cmake/pwr-probe-test/default/user.cmake`
- `cmake/pwr-probe-test/default.production/user.cmake`

They add device macros, XC16 include dirs, FreeRTOS paths, and
`-mno-eds-warn` to the compile targets. If MPLAB regenerates CMake and includes
disappear, check these files first.

Generated source lists currently include `src/main.c` and `src/pwm.c` in both:
- `cmake/pwr-probe-test/default/.generated/file.cmake`
- `cmake/pwr-probe-test/default.production/.generated/file.cmake`

`user.cmake` adds display sources (`src/st7789v.c` and
`src/st7789v_demo.c`) to both configs.

If the production build complains about missing root-level `main.c`, regenerate
or repair the production generated source list.

### IntelliSense

- `xc.h` resolves through `.vscode/c_cpp_properties.json` include paths.
- Device macros are explicit in the IntelliSense defines because XC16 injects
  them from `-mcpu`, but cpptools does not.
- Do not add `"C_Cpp.default.configurationProvider": "microchip.mplab-clangd"`
  to `.vscode/settings.json`. MPLAB clangd can override include paths with an
  empty config until its binary downloads, breaking IntelliSense.

## Key Files

| Path | Purpose |
|------|---------|
| `.vscode/pwr-probe-test.mplab.json` | MPLAB project config; do not delete; uses `**/*` glob to pick up sources |
| `src/main.c` | Config bits, PWM LED setup, FreeRTOS task startup |
| `src/pwm.h`, `src/pwm.c` | Timer2/OC1-OC4 PWM driver |
| `src/st7789v.h`, `src/st7789v.c` | ST7789V display API and PIC24 SPI/GPIO backend |
| `src/amazing_utils.h` | Shared aliases, macros, GPIO abstraction |
| `FreeRTOSConfig.h` | FreeRTOS config for this MCU |
| `cmake/pwr-probe-test/default/user.cmake` | Debug build customisation |
| `cmake/pwr-probe-test/default.production/user.cmake` | Production build customisation; keep in sync with debug |
| `FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/portmacro.h` | Local patch: includes `xc.h` for `SET_CPU_IPL`; guards `SIZE_MAX` |
| `FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/port.c` | Local patch: includes `xc.h` and `libpic30.h` for SFR access |
| `out/` | Build artifacts |
| `_build/`, `cmake/` | Generated CMake/build tree; deletable, but preserve `user.cmake` |
