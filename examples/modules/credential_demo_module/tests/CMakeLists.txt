set(MODULE_NAME credential_demo_module)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_credential_demo_module.cpp
                 test_app.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
)

if (DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH)
    target_compile_definitions(${TEST_APP} PRIVATE DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH)
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
