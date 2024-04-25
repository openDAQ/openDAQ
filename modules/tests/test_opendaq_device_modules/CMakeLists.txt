set(TEST_APP test_device_modules)

set(TEST_SOURCES test_native_device_modules.cpp
                 test_opcua_device_modules.cpp
                 test_app.cpp
                 test_helpers.h
)

if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    list(APPEND TEST_SOURCES test_websocket_modules.cpp)
endif()

if (OPENDAQ_ENABLE_NATIVE_STREAMING)
    list(APPEND TEST_SOURCES test_native_streaming_modules.cpp)
endif()

if (OPENDAQ_ENABLE_NATIVE_STREAMING OR OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    list(APPEND TEST_SOURCES test_subdevices.cpp test_streaming.cpp)
endif()

add_executable(${TEST_APP} ${TEST_SOURCES}
)

if("${Boost_LIBRARIES}")
    set(BOOST_LIBS ${Boost_LIBRARIES})
endif()


target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
                                          daq::opcuatms_client
                                          daq::opendaq_mocks
                                          ${BOOST_LIBS}
)

# Should link only what you need $<BUILD_INTERFACE:Boost::asio>
target_link_libraries(${TEST_APP} PRIVATE $<BUILD_INTERFACE:Boost::asio>)


add_dependencies(${TEST_APP} daq::opcua_client_module
                             daq::opcua_server_module
                             daq::ref_fb_module
                             daq::ref_device_module
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
         COMMAND $<TARGET_FILE:${TEST_APP}>
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(${TEST_APP} PRIVATE OPENDAQ_SKIP_DLL_IMPORT)

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
