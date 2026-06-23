set(DEPENDENT_MP_BIN2HEXpwr_probe_test_default_NY8ttQuG "c:/Program Files/Microchip/xc16/v2.10/bin/xc16-bin2hex.exe")
set(DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_NY8ttQuG "${CMAKE_CURRENT_LIST_DIR}/../../../../out/pwr-probe-test/production/default-production.elf")
set(DEPENDENT_TARGET_DIRpwr_probe_test_default_NY8ttQuG "${CMAKE_CURRENT_LIST_DIR}/../../../../out/pwr-probe-test/production")
set(DEPENDENT_BYPRODUCTSpwr_probe_test_default_NY8ttQuG ${DEPENDENT_TARGET_DIRpwr_probe_test_default_NY8ttQuG}/${sourceFileNamepwr_probe_test_default_NY8ttQuG}.s)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRpwr_probe_test_default_NY8ttQuG}/${sourceFileNamepwr_probe_test_default_NY8ttQuG}.s
    COMMAND ${DEPENDENT_MP_BIN2HEXpwr_probe_test_default_NY8ttQuG} ${DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_NY8ttQuG} --image ${sourceFileNamepwr_probe_test_default_NY8ttQuG} ${addresspwr_probe_test_default_NY8ttQuG} ${modepwr_probe_test_default_NY8ttQuG} -mdfp=C:/Users/Apollo Prog/.mchp_packs/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRpwr_probe_test_default_NY8ttQuG}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_NY8ttQuG})
add_custom_target(
    dependent_produced_source_artifactpwr_probe_test_default_NY8ttQuG 
    DEPENDS ${DEPENDENT_TARGET_DIRpwr_probe_test_default_NY8ttQuG}/${sourceFileNamepwr_probe_test_default_NY8ttQuG}.s
    )
