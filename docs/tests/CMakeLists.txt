set_cmake_folder_context(TARGET_FOLDER_NAME tests)

set(TEST_APP test_docs)

set(TEST_SOURCES docs_test_helpers.h
                 test_quick_start.cpp
                 test_app.cpp
                 test_property_system.cpp
                 test_examples.cpp
                 test_howto.cpp
                 test_architecture.cpp
                 test_modules.cpp
                 test_pointers.cpp
                 test_components.cpp
                 test_signals.cpp
                 test_devices.cpp
                 test_config_provider.cpp
                 test_get_last_value.cpp
                 test_howto_configure_instance.cpp
)

if (OPENDAQ_ENABLE_NATIVE_STREAMING AND OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    list(APPEND TEST_SOURCES test_streaming_config.cpp)
endif()

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::opendaq
                                          daq::test_utils
)

add_dependencies(${TEST_APP} daq::ref_fb_module
                             daq::ref_device_module
                             daq::opcua_client_module
                             daq::opcua_server_module
)
if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    add_dependencies(${TEST_APP} daq::ws_stream_cl_module
                                 daq::ws_stream_srv_module
    )
endif()
if (OPENDAQ_ENABLE_NATIVE_STREAMING)
    add_dependencies(${TEST_APP} daq::native_stream_cl_module
                                 daq::native_stream_srv_module
    )
endif()


add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
