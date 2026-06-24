cmake_minimum_required(VERSION 3.20)
project(pwr-probe-test C ASM)

set(DFP /Users/misikovich/.mchp_packs/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16)

set(MCU_FLAGS
    -mcpu=24EP256MC206
    -mdfp=${DFP}
)

set(COMMON_FLAGS
    ${MCU_FLAGS}
    -DXPRJ_default=default
    -D__DEBUG
    -D__PIC24EP256MC206__
    -D__24EP256MC206__
    -D__PIC24E__
    -g
    -O0
    -msmart-io=1
    -Wall
    -msfr-warn=off
    -mno-eds-warn
)

file(GLOB APP_SOURCES CONFIGURE_DEPENDS src/*.c)

set(FREERTOS_SOURCES
    FreeRTOS/Source/tasks.c
    FreeRTOS/Source/queue.c
    FreeRTOS/Source/list.c
    FreeRTOS/Source/event_groups.c
    FreeRTOS/Source/stream_buffer.c
    FreeRTOS/Source/timers.c
    FreeRTOS/Source/portable/MemMang/heap_4.c
    FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/port.c
    FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/portasm_PIC24.S
)

add_executable(${PROJECT_NAME}.elf
    ${APP_SOURCES}
    ${FREERTOS_SOURCES}
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/FreeRTOS/Source/include
    ${CMAKE_SOURCE_DIR}/FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE ${COMMON_FLAGS})

set(LINKER_SCRIPT ${DFP}/support/PIC24E/gld/p24EP256MC206.gld)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -Wl,--heap=512
    -Wl,--check-sections
    -Wl,--script=${LINKER_SCRIPT}
)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND /Applications/microchip/xc16/v2.10/bin/xc16-bin2hex
            ${CMAKE_SOURCE_DIR}/out/${PROJECT_NAME}.elf
            -a -omf=elf
    COMMENT "Generating HEX: out/${PROJECT_NAME}.hex"
)
