set(MODULE_NAME native_streaming_protocol)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_types.cpp
                 test_base.h
                 test_config_packets.cpp
                 test_streaming_protocol.cpp
                 test_client_to_dev_streaming.cpp
                 test_tls_transport.cpp
)

add_executable(${TEST_APP} test_app.cpp
                           ${TEST_SOURCES}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::config_protocol
                                          daq::test_utils
					  daq::opendaq
					  daq::opendaq_mocks
)

# fixtures for the TLS transport tests, from libNativeStreaming rather than a second copy here
if (NATIVE_STREAMING_ENABLE_TLS AND DEFINED native_streaming_SOURCE_DIR)
    set(NATIVE_TLS_SECRETS_DIR "${native_streaming_SOURCE_DIR}/test/secrets")
    if (EXISTS "${NATIVE_TLS_SECRETS_DIR}")
        add_custom_command(TARGET ${TEST_APP} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${NATIVE_TLS_SECRETS_DIR}
                    $<TARGET_FILE_DIR:${TEST_APP}>/native_secrets)
    else()
        message(WARNING "TLS fixtures not found in ${NATIVE_TLS_SECRETS_DIR}. "
                        "The native streaming TLS transport tests will fail to load their secrets.")
    endif()
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
