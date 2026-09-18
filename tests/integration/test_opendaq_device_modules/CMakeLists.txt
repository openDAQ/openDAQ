set(TEST_HEADERS
    test_helpers/test_helpers.h
    test_helpers/device_modules.h
    test_helpers/lt_tls.h
)

# The tests that advertise over mDNS or scan for devices: one binary, one ctest entry holding every network lock
set(TEST_APP test_device_modules)
set(TEST_SOURCES test_native_device_modules.cpp
                 test_opcua_device_modules.cpp
                 test_default_config.cpp
                 test_app.cpp
                 test_websocket_modules.cpp
                 test_native_streaming_modules.cpp
                 test_device_discovery.cpp
                 test_remote_update.cpp
)

# The tests that never touch mDNS: gtest shards on shifted ports, running concurrently with everything else
set(STREAMING_TEST_APP test_device_modules_streaming)
set(STREAMING_TEST_SOURCES test_app.cpp
                           test_streaming.cpp
                           test_subdevices.cpp
                           test_device_locking.cpp
)
set(STREAMING_TEST_SHARDS 4)

function(add_device_modules_test_app APP)
    add_executable(${APP}
        ${TEST_HEADERS}
        ${ARGN}
    )

    target_link_libraries(${APP} PRIVATE daq::test_utils
                                         daq::opendaq
                                         daq::opcuatms_client
                                         daq::native_stream_cl_module
                                         daq::opendaq_mocks
    )

    # Should link only what you need $<BUILD_INTERFACE:Boost::asio>
    target_link_libraries(${APP} PRIVATE $<BUILD_INTERFACE:Boost::asio>
                                         $<BUILD_INTERFACE:std::filesystem>)

    if (MINGW)
        target_link_libraries(${APP} PRIVATE ws2_32)
    endif()

    target_link_libraries(${APP}
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
        target_compile_options(${APP} PRIVATE /bigobj)
    endif()

    opendaq_target_pch(${APP} <opendaq/opendaq.h> <gtest/gtest.h>)
    opendaq_target_unity_tests(${APP} BATCH_SIZE 4)

    target_compile_definitions(${APP} PRIVATE OPENDAQ_SKIP_DLL_IMPORT)

    set_target_properties(${APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${APP}>)

    if (OPENDAQ_ENABLE_COVERAGE)
        setup_target_for_coverage(${APP}coverage ${APP} ${APP}coverage)
    endif()
endfunction()

# Too large to merge into a unity translation unit: MSVC hits an internal compiler error.
set_source_files_properties(test_native_device_modules.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

add_device_modules_test_app(${TEST_APP} ${TEST_SOURCES})
add_device_modules_test_app(${STREAMING_TEST_APP} ${STREAMING_TEST_SOURCES})

# The LT streaming tests over the secure channel need TLS fixtures (CA, server and client
# certificates/keys). Reuse the pre-generated ones from the LT streaming modules repository instead of
# keeping a second copy here. The legacy LT modules have neither TLS support nor those fixtures, and
# neither has a modern build with the secure channel switched off.
if (NOT DAQMODULES_LT_LEGACY_MODULES AND OPENDAQ_ENABLE_WEBSOCKET_STREAMING_WITH_TLS AND DEFINED LT_MODULES_SOURCE_DIR)
    set(LT_TLS_SECRETS_DIR "${LT_MODULES_SOURCE_DIR}/tests/integration/secrets")
    if (EXISTS "${LT_TLS_SECRETS_DIR}")
        foreach(APP ${TEST_APP} ${STREAMING_TEST_APP})
            add_custom_command(TARGET ${APP} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${LT_TLS_SECRETS_DIR}
                        $<TARGET_FILE_DIR:${APP}>/secrets)
        endforeach()
    else()
        message(WARNING "TLS fixtures not found in ${LT_TLS_SECRETS_DIR}. "
                        "The LT streaming tests over the secure channel will fail to load their secrets.")
    endif()
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)
opendaq_test_resource_lock(${TEST_APP} network_native network_lt network_opcua mdns)

opendaq_add_test_shards(${STREAMING_TEST_APP} ${STREAMING_TEST_SHARDS})
