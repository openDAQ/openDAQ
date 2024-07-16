set(MODULE_NAME opcua_server_module)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_opcua_server_module.cpp
                 test_app.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opcuaclient
                                          daq::opendaq_mocks
                                          ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          Taskflow::Taskflow
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (MSVC) # Ignoring warning for the Taskflow
    target_compile_options(${TEST_APP} PRIVATE /wd4324)
endif()

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
