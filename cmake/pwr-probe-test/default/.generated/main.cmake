include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(pwr_probe_test_default_library_list )

# Handle files with suffix s, for group default-XC16
if(pwr_probe_test_default_default_XC16_FILE_TYPE_assemble)
add_library(pwr_probe_test_default_default_XC16_assemble OBJECT ${pwr_probe_test_default_default_XC16_FILE_TYPE_assemble})
    pwr_probe_test_default_default_XC16_assemble_rule(pwr_probe_test_default_default_XC16_assemble)
    list(APPEND pwr_probe_test_default_library_list "$<TARGET_OBJECTS:pwr_probe_test_default_default_XC16_assemble>")

endif()

# Handle files with suffix S, for group default-XC16
if(pwr_probe_test_default_default_XC16_FILE_TYPE_assemblePreproc)
add_library(pwr_probe_test_default_default_XC16_assemblePreproc OBJECT ${pwr_probe_test_default_default_XC16_FILE_TYPE_assemblePreproc})
    pwr_probe_test_default_default_XC16_assemblePreproc_rule(pwr_probe_test_default_default_XC16_assemblePreproc)
    list(APPEND pwr_probe_test_default_library_list "$<TARGET_OBJECTS:pwr_probe_test_default_default_XC16_assemblePreproc>")

endif()

# Handle files with suffix c, for group default-XC16
if(pwr_probe_test_default_default_XC16_FILE_TYPE_compile)
add_library(pwr_probe_test_default_default_XC16_compile OBJECT ${pwr_probe_test_default_default_XC16_FILE_TYPE_compile})
    pwr_probe_test_default_default_XC16_compile_rule(pwr_probe_test_default_default_XC16_compile)
    list(APPEND pwr_probe_test_default_library_list "$<TARGET_OBJECTS:pwr_probe_test_default_default_XC16_compile>")

endif()

# Handle files with suffix s, for group default-XC16
if(pwr_probe_test_default_default_XC16_FILE_TYPE_dependentObject)
add_library(pwr_probe_test_default_default_XC16_dependentObject OBJECT ${pwr_probe_test_default_default_XC16_FILE_TYPE_dependentObject})
    pwr_probe_test_default_default_XC16_dependentObject_rule(pwr_probe_test_default_default_XC16_dependentObject)
    list(APPEND pwr_probe_test_default_library_list "$<TARGET_OBJECTS:pwr_probe_test_default_default_XC16_dependentObject>")

endif()

# Handle files with suffix elf, for group default-XC16
if(pwr_probe_test_default_default_XC16_FILE_TYPE_bin2hex)
add_library(pwr_probe_test_default_default_XC16_bin2hex OBJECT ${pwr_probe_test_default_default_XC16_FILE_TYPE_bin2hex})
    pwr_probe_test_default_default_XC16_bin2hex_rule(pwr_probe_test_default_default_XC16_bin2hex)
    list(APPEND pwr_probe_test_default_library_list "$<TARGET_OBJECTS:pwr_probe_test_default_default_XC16_bin2hex>")

endif()


# Main target for this project
add_executable(pwr_probe_test_default_image_PdE98j9L ${pwr_probe_test_default_library_list})

set_target_properties(pwr_probe_test_default_image_PdE98j9L PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY "${pwr_probe_test_default_output_dir}")
target_link_libraries(pwr_probe_test_default_image_PdE98j9L PRIVATE ${pwr_probe_test_default_default_XC16_FILE_TYPE_link})

# Add the link options from the rule file.
pwr_probe_test_default_link_rule( pwr_probe_test_default_image_PdE98j9L)

# Call bin2hex function from the rule file
pwr_probe_test_default_bin2hex_rule(pwr_probe_test_default_image_PdE98j9L)

