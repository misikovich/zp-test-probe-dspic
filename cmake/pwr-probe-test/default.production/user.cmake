# XC16 derives __PIC24EP256MC206__, __24EP256MC206__, and __PIC24E__ from
# -mcpu=24EP256MC206 at compile time, but cpptools/clangd do not interpret
# -mcpu flags. XC16 also resolves support/generic/h and the DFP paths
# implicitly; adding them explicitly makes IntelliSense find xc.h and the
# device SFR headers.
#
# FreeRTOS include paths are also added here so the kernel sources can
# resolve FreeRTOS.h, portmacro.h, and FreeRTOSConfig.h.

set(_FREERTOS_ROOT "${CMAKE_SOURCE_DIR}/../../../FreeRTOS")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_XC16_ROOT "/Applications/microchip/xc16/v2.10")
    set(_MCHP_PACKS_ROOT "$ENV{HOME}/.mchp_packs")
else()
    set(_XC16_ROOT "C:/Program Files/Microchip/xc16/v2.10")
    set(_MCHP_PACKS_ROOT "C:/Users/Apollo Prog/.mchp_packs")
endif()

set(_DFP_XC16_ROOT
    "${_MCHP_PACKS_ROOT}/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16")

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
            "${_XC16_ROOT}/support/generic/h"
            "${_XC16_ROOT}/support/PIC24E/h"
            "${_XC16_ROOT}/include"
            "${_DFP_XC16_ROOT}/support/PIC24E/h"
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
    set(_fonts_source "${CMAKE_SOURCE_DIR}/../../../src/fonts.c")
    set(_fonts_index -1)
    get_target_property(_compile_sources
        pwr_probe_test_default_default_XC16_compile SOURCES)
    if(_compile_sources)
        list(REMOVE_ITEM _compile_sources
            "${CMAKE_SOURCE_DIR}/../../../src/st7789v.c"
            "${CMAKE_SOURCE_DIR}/../../../src/st7789v_demo.c")
        list(FIND _compile_sources "${_fonts_source}" _fonts_index)
        set_target_properties(pwr_probe_test_default_default_XC16_compile
            PROPERTIES SOURCES "${_compile_sources}")
    endif()

    target_sources(pwr_probe_test_default_default_XC16_compile PRIVATE
        "${CMAKE_SOURCE_DIR}/../../../src/motor.c"
        "${CMAKE_SOURCE_DIR}/../../../src/display_hw.c")

    if(_fonts_index EQUAL -1)
        target_sources(pwr_probe_test_default_default_XC16_compile PRIVATE
            "${_fonts_source}")
    endif()
endif()
