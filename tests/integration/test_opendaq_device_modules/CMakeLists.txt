set(TEST_APP test_device_modules)

set(TEST_HEADERS
    test_helpers/test_helpers.h
    test_helpers/device_modules.h
    test_helpers/lt_tls.h
)

set(TEST_SOURCES test_native_device_modules.cpp
                 test_opcua_device_modules.cpp
                 test_default_config.cpp
                 test_device_locking.cpp
                 test_app.cpp                 
                 test_websocket_modules.cpp
                 test_native_streaming_modules.cpp
                 test_subdevices.cpp
                 test_streaming.cpp
                 test_device_discovery.cpp
                 test_remote_update.cpp
)

add_executable(${TEST_APP}
    ${TEST_HEADERS}
    ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
                                          daq::opcuatms_client
                                          daq::native_stream_cl_module
                                          daq::opendaq_mocks
)

# Should link only what you need $<BUILD_INTERFACE:Boost::asio>
target_link_libraries(${TEST_APP} PRIVATE $<BUILD_INTERFACE:Boost::asio>
                                          $<BUILD_INTERFACE:std::filesystem>)

if (MINGW)
    target_link_libraries(${TEST_APP} PRIVATE ws2_32)
endif()

target_link_libraries(${TEST_APP}
    PRIVATE
        daq::opcua_client_module
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

# The LT streaming tests over the secure channel need TLS fixtures (CA, server and client
# certificates/keys). Reuse the pre-generated ones from the LT streaming modules repository instead of
# keeping a second copy here. The legacy LT modules have neither TLS support nor those fixtures.
if (NOT DAQMODULES_LT_LEGACY_MODULES AND DEFINED LT_MODULES_SOURCE_DIR)
    set(LT_TLS_SECRETS_DIR "${LT_MODULES_SOURCE_DIR}/tests/integration/secrets")
    if (EXISTS "${LT_TLS_SECRETS_DIR}")
        add_custom_command(TARGET ${TEST_APP} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${LT_TLS_SECRETS_DIR}
                    $<TARGET_FILE_DIR:${TEST_APP}>/secrets)
    else()
        message(WARNING "TLS fixtures not found in ${LT_TLS_SECRETS_DIR}. "
                        "The LT streaming tests over the secure channel will fail to load their secrets.")
    endif()
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

target_compile_definitions(${TEST_APP} PRIVATE OPENDAQ_SKIP_DLL_IMPORT)

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
