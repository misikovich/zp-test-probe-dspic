# XC16 derives __PIC24EP256MC206__, __24EP256MC206__, and __PIC24E__ from
# -mcpu=24EP256MC206 at compile time, but cpptools/clangd do not interpret
# -mcpu flags. XC16 also resolves support/generic/h and the DFP paths
# implicitly; adding them explicitly makes IntelliSense find xc.h and the
# device SFR headers.
#
# FreeRTOS include paths are also added here so the kernel sources can
# resolve FreeRTOS.h, portmacro.h, and FreeRTOSConfig.h.

set(_FREERTOS_ROOT "${CMAKE_SOURCE_DIR}/../../../FreeRTOS")

foreach(_t
    pwr_probe_test_default_default_XC16_compile
    pwr_probe_test_default_default_XC16_assemble
    pwr_probe_test_default_default_XC16_assemblePreproc)
    if(TARGET ${_t})
        target_compile_definitions(${_t} PRIVATE
            "__PIC24EP256MC206__"
            "__24EP256MC206__"
            "__PIC24E__")
        target_include_directories(${_t} PRIVATE
            # XC16 implicit paths
            "C:/Program Files/Microchip/xc16/v2.10/support/generic/h"
            "C:/Program Files/Microchip/xc16/v2.10/support/PIC24E/h"
            "C:/Program Files/Microchip/xc16/v2.10/include"
            "C:/Users/Apollo Prog/.mchp_packs/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16/support/PIC24E/h"
            "C:/Users/Apollo Prog/.mchp_packs/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16/include"
            # Project root — FreeRTOSConfig.h lives here
            "${CMAKE_SOURCE_DIR}/../../../"
            # FreeRTOS kernel headers
            "${_FREERTOS_ROOT}/Source/include"
            # PIC24/dsPIC port header (portmacro.h)
            "${_FREERTOS_ROOT}/Source/portable/MPLAB/PIC24_dsPIC")
        # Suppress PIC24E EDS "extended pointer" warnings in FreeRTOS sources
        # (stack variables passed by pointer are always in near memory — safe)
        target_compile_options(${_t} PRIVATE -mno-eds-warn)
    endif()
endforeach()

if(TARGET pwr_probe_test_default_default_XC16_compile)
    target_sources(pwr_probe_test_default_default_XC16_compile PRIVATE
        "${CMAKE_SOURCE_DIR}/../../../src/motor.c"
        "${CMAKE_SOURCE_DIR}/../../../src/st7789v.c"
        "${CMAKE_SOURCE_DIR}/../../../src/st7789v_demo.c")
endif()
