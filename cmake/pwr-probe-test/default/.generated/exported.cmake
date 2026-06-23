set(DEPENDENT_MP_BIN2HEXpwr_probe_test_default_PdE98j9L "/Applications/microchip/xc16/v2.10/bin/xc16-bin2hex")
set(DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_PdE98j9L ${CMAKE_CURRENT_LIST_DIR}/../../../../out/pwr-probe-test/default.elf)
set(DEPENDENT_TARGET_DIRpwr_probe_test_default_PdE98j9L ${CMAKE_CURRENT_LIST_DIR}/../../../../out/pwr-probe-test)
set(DEPENDENT_BYPRODUCTSpwr_probe_test_default_PdE98j9L ${DEPENDENT_TARGET_DIRpwr_probe_test_default_PdE98j9L}/${sourceFileNamepwr_probe_test_default_PdE98j9L}.s)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRpwr_probe_test_default_PdE98j9L}/${sourceFileNamepwr_probe_test_default_PdE98j9L}.s
    COMMAND ${DEPENDENT_MP_BIN2HEXpwr_probe_test_default_PdE98j9L} ${DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_PdE98j9L} --image ${sourceFileNamepwr_probe_test_default_PdE98j9L} ${addresspwr_probe_test_default_PdE98j9L} ${modepwr_probe_test_default_PdE98j9L} -mdfp=/Users/misikovich/.mchp_packs/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRpwr_probe_test_default_PdE98j9L}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_PdE98j9L})
add_custom_target(
    dependent_produced_source_artifactpwr_probe_test_default_PdE98j9L 
    DEPENDS ${DEPENDENT_TARGET_DIRpwr_probe_test_default_PdE98j9L}/${sourceFileNamepwr_probe_test_default_PdE98j9L}.s
    )
