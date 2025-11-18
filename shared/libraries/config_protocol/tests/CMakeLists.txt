set(BASE_NAME config_protocol)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES
    test_config_packet.cpp
    test_config_client_server.cpp
    test_config_protocol_integration.cpp
    test_config_protocol_device_locking.cpp
    test_config_protocol_view_only_client.cpp
    test_config_serialization.cpp
    test_ref_modules.cpp
    test_app.cpp
    test_core_events.cpp
    test_nested_property_objects.cpp
    test_remote_update.cpp
    test_c2d_streaming.cpp
)

if (OPENDAQ_ENABLE_ACCESS_CONTROL)
    list(APPEND TEST_SOURCES test_config_protocol_access_control.cpp)
endif()

add_executable(${TEST_APP} ${TEST_SOURCES})

target_link_libraries(${TEST_APP} PRIVATE
    ${SDK_TARGET_NAMESPACE}::${BASE_NAME}
    ${SDK_TARGET_NAMESPACE}::test_utils testlibs
    daq::opendaq
    daq::opendaq_mocks
)

if (DAQMODULES_REF_FB_MODULE AND DAQMODULES_REF_DEVICE_MODULE)
    add_dependencies(${TEST_APP}
        ${SDK_TARGET_NAMESPACE}::ref_device_module
        ${SDK_TARGET_NAMESPACE}::ref_fb_module
    )

    target_compile_definitions(${TEST_APP} PRIVATE OPENDAQ_TEST_WITH_REF_MODULES)
else()
    message(WARNING "Disabling tests for Reference Device and Function Blocks as they are disabled.")
endif()

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

add_test(NAME ${TEST_APP}
    COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

if(OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
