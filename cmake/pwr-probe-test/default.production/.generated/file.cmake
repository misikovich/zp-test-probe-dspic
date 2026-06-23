# The following variables contains the files used by the different stages of the build process.
set(pwr_probe_test_default_default_XC16_FILE_TYPE_assemble)
set_source_files_properties(${pwr_probe_test_default_default_XC16_FILE_TYPE_assemble} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${pwr_probe_test_default_default_XC16_FILE_TYPE_assemble})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(pwr_probe_test_default_default_XC16_FILE_TYPE_assemblePreproc "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/portasm_PIC24.S")
set_source_files_properties(${pwr_probe_test_default_default_XC16_FILE_TYPE_assemblePreproc} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${pwr_probe_test_default_default_XC16_FILE_TYPE_assemblePreproc})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(pwr_probe_test_default_default_XC16_FILE_TYPE_compile
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/event_groups.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/list.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/portable/MPLAB/PIC24_dsPIC/port.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/portable/MemMang/heap_4.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/queue.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/stream_buffer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/tasks.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../FreeRTOS/Source/timers.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/pwm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/st7789v.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/st7789v_demo.c")
set_source_files_properties(${pwr_probe_test_default_default_XC16_FILE_TYPE_compile} PROPERTIES LANGUAGE C)
set(pwr_probe_test_default_default_XC16_FILE_TYPE_link)
set(pwr_probe_test_default_default_XC16_FILE_TYPE_bin2hex)
set(pwr_probe_test_default_image_name "default-production.elf")
set(pwr_probe_test_default_image_base_name "default-production")

# The output directory of the final image.
set(pwr_probe_test_default_output_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../out/pwr-probe-test/production")

# The full path to the final image.
set(pwr_probe_test_default_full_path_to_image ${pwr_probe_test_default_output_dir}/${pwr_probe_test_default_image_name})
