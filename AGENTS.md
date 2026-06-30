# Repository Guidelines

## Project Structure & Module Organization

This is an MPLAB/XC16 embedded C project for a PIC24EP256MC206 EV charger service tester. Application code lives in `src/`; `main.c` initializes clock, hardware, FreeRTOS tasks, and page UI. Keep hardware modules paired as `name.c`/`name.h` such as `motor`, `rgbw`, `sensors`, `bcd_switch`, `uart`, and `display_hw`. Shared project headers are at the repo root (`FreeRTOSConfig.h`) and in `include/` (`sound_protocol.h`). Vendored kernel code is under `FreeRTOS/Source/`; avoid editing it except for known PIC24 port fixes. Build output belongs in `out/` and `_build/`. Host tools live in `tools/`, including `png2h.py` for RGB565 display assets.

## Build, Test, and Development Commands

- `Ctrl+Shift+B` in VS Code: run the MPLAB extension build.
- `& "C:\Users\Apollo Prog\.mplab\app-finder\apps\ninja\v1.13.2\ninja.exe" -C "_build\pwr-probe-test\default"`: build the default configuration from PowerShell.
- `.\flash.ps1`: flash `out\pwr-probe-test\default.hex` with PICkit 4.
- `python tools\png2h.py tf.png -o src\startup_logo.h -n startup_logo`: convert a PNG to a byte-swapped RGB565 header.

## Coding Style & Naming Conventions

XC16 compiles C files in C89/C90 mode. Use `/* */` comments, fixed-size integer types, `static` helpers for file-local functions, and no VLAs. Match the existing 4-space indentation and brace style. Constants and macros use uppercase names (`APP_DISPLAY_PERIOD_MS`); functions use lower_snake_case or existing module prefixes (`mtr_`, `rgbw_`, `gfx_`, `st_`). Prefer the GPIO/ADC helpers in `src/hardware.h` over direct register access outside low-level drivers.

## Testing Guidelines

There is no standalone unit-test suite yet. Treat a clean XC16 build as the minimum check, and run a single-file compile for risky C changes when full hardware builds are slow. For behavior changes, test on target hardware when possible and note observed display, motor, UART, sensor, or LED behavior in the PR.

## Commit & Pull Request Guidelines

Recent commits use short imperative summaries, with occasional Conventional Commit prefixes such as `feat:`. Keep subjects concise, for example `feat: smooth rgbw fade api` or `add bcd switch polling`. PRs should describe the hardware area touched, list build or flash checks performed, link related issues, and include screenshots or photos for display/UI changes.

## Configuration & Safety Notes

Keep MPLAB-generated config files and `cmake/.../user.cmake` changes intentional. Do not commit local build artifacts from `out/`, `_build/`, object files, or generated experiments unless they are required source assets.
