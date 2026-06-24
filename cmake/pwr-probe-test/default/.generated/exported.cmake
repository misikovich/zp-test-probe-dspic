set(DEPENDENT_MP_BIN2HEXpwr_probe_test_default_FxBTO9fd "c:/Program Files/Microchip/xc16/v2.10/bin/xc16-bin2hex.exe")
set(DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_FxBTO9fd "${CMAKE_CURRENT_LIST_DIR}/../../../../out/pwr-probe-test/default.elf")
set(DEPENDENT_TARGET_DIRpwr_probe_test_default_FxBTO9fd "${CMAKE_CURRENT_LIST_DIR}/../../../../out/pwr-probe-test")
set(DEPENDENT_BYPRODUCTSpwr_probe_test_default_FxBTO9fd ${DEPENDENT_TARGET_DIRpwr_probe_test_default_FxBTO9fd}/${sourceFileNamepwr_probe_test_default_FxBTO9fd}.s)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRpwr_probe_test_default_FxBTO9fd}/${sourceFileNamepwr_probe_test_default_FxBTO9fd}.s
    COMMAND ${DEPENDENT_MP_BIN2HEXpwr_probe_test_default_FxBTO9fd} ${DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_FxBTO9fd} --image ${sourceFileNamepwr_probe_test_default_FxBTO9fd} ${addresspwr_probe_test_default_FxBTO9fd} ${modepwr_probe_test_default_FxBTO9fd} -mdfp=C:/Users/Apollo Prog/.mchp_packs/Microchip/dsPIC33E-GM-GP-MC-GU-MU_DFP/1.6.297/xc16 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRpwr_probe_test_default_FxBTO9fd}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFpwr_probe_test_default_FxBTO9fd})
add_custom_target(
    dependent_produced_source_artifactpwr_probe_test_default_FxBTO9fd 
    DEPENDS ${DEPENDENT_TARGET_DIRpwr_probe_test_default_FxBTO9fd}/${sourceFileNamepwr_probe_test_default_FxBTO9fd}.s
    )
