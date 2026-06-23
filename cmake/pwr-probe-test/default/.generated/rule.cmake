# The following functions contains all the flags passed to the different build stages.

set(PACK_REPO_PATH "/Users/misikovich/.mchp_packs" CACHE PATH "Path to the root of a pack repository.")

function(pwr_probe_test_default_default_XC16_assemble_rule target)
    set(options
        "-g"
        "-mcpu=24EP256MC206"
        "-Wa,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-g,--no-relax"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=1"
        PRIVATE "XPRJ_default=default")
endfunction()
function(pwr_probe_test_default_default_XC16_assemblePreproc_rule target)
    set(options
        "-x"
        "assembler-with-cpp"
        "-g"
        "-mcpu=24EP256MC206"
        "-Wa,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-g,--no-relax"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=1"
        PRIVATE "XPRJ_default=default")
endfunction()
function(pwr_probe_test_default_default_XC16_compile_rule target)
    set(options
        "-g"
        "-mcpu=24EP256MC206"
        "-O0"
        "-msmart-io=1"
        "-Wall"
        "-msfr-warn=off"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG"
        PRIVATE "XPRJ_default=default")
endfunction()
function(pwr_probe_test_default_dependentObject_rule target)
    set(options
        "-c"
        "-mcpu=24EP256MC206"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
endfunction()
function(pwr_probe_test_default_link_rule target)
    set(options
        "-g"
        "-mcpu=24EP256MC206"
        "-Wl,--script=p24EP256MC206.gld,--local-stack,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,--report-mem,--memorysummary,memoryfile.xml"
        "-mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16")
    list(REMOVE_ITEM options "")
    target_link_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=__DEBUG"
        PRIVATE "XPRJ_default=default")
endfunction()
function(pwr_probe_test_default_bin2hex_rule target)
    add_custom_target(
        pwr_probe_test_default_Bin2Hex ALL
        COMMAND ${MP_BIN2HEX} ${pwr_probe_test_default_image_name} -a -mdfp=${PACK_REPO_PATH}/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16
        WORKING_DIRECTORY ${pwr_probe_test_default_output_dir}
        BYPRODUCTS "${pwr_probe_test_default_output_dir}/${pwr_probe_test_default_image_base_name}.hex"
        COMMENT "Convert build file to .hex")
    add_dependencies(pwr_probe_test_default_Bin2Hex ${target})
endfunction()
