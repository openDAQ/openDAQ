set(MODULE_NAME opcua)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES
    main.cpp
    test_opcua_security.cpp
    test_opcua_authentication.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES})

target_link_libraries(${TEST_APP} PRIVATE opcuashared
                                          opcuaserver
                                          opcuaclient
                                          daq::test_utils
)

if (MSVC)
    target_compile_definitions(${TEST_APP} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}testcoverage ${TEST_APP} ${MODULE_NAME}testcoverage)
endif()
