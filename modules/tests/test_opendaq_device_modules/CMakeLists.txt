set(TEST_APP test_device_modules)

set(TEST_SOURCES test_native_device_modules.cpp
                 test_opcua_device_modules.cpp
                 test_default_config.cpp
                 test_device_locking.cpp
                 test_app.cpp
                 test_helpers/test_helpers.h
                 test_websocket_modules.cpp
                 test_native_streaming_modules.cpp
                 test_subdevices.cpp
                 test_streaming.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
                                          daq::opcuatms_client
                                          daq::native_stream_cl_module
                                          daq::opendaq_mocks
)

# Should link only what you need $<BUILD_INTERFACE:Boost::asio>
target_link_libraries(${TEST_APP} PRIVATE $<BUILD_INTERFACE:Boost::asio>)


add_dependencies(${TEST_APP} daq::opcua_client_module
                             daq::opcua_server_module
                             daq::ref_fb_module
                             daq::ref_device_module
                             daq::ws_stream_cl_module
                             daq::ws_stream_srv_module
                             daq::native_stream_cl_module
                             daq::native_stream_srv_module
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

target_compile_definitions(${TEST_APP} PRIVATE OPENDAQ_SKIP_DLL_IMPORT)

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
