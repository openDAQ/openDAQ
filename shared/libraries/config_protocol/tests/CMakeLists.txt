set(BASE_NAME config_protocol)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})
set(TEST_APP test_${MODULE_NAME})

add_executable(${TEST_APP}
    test_config_packet.cpp
    test_config_client_server.cpp
    test_config_protocol_integration.cpp
	test_config_serialization.cpp
    test_ref_modules.cpp
    test_app.cpp
    test_core_events.cpp
    test_nested_property_objects.cpp
    test_utils.h
    test_utils.cpp
)

target_link_libraries(${TEST_APP} PRIVATE
    ${SDK_TARGET_NAMESPACE}::${BASE_NAME}
    ${SDK_TARGET_NAMESPACE}::test_utils
    daq::opendaq
    daq::opendaq_mocks
    GTest::GTest GTest::gmock
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

add_test(NAME ${TEST_APP}
    COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
    WORKING_DIRECTORY bin
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

if(OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
