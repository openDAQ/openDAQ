set(MODULE_NAME native_stream_srv_module)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_native_streaming_server_module.cpp
                 test_app.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES}
)

# the protocol library is linked for the NATIVE_STREAMING_ENABLE_TLS definition it carries, which the
# module keeps to itself
target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq_mocks
                                          daq::native_streaming_protocol
                                          ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          Taskflow
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (MSVC) # Ignoring warning for the Taskflow
    target_compile_options(${TEST_APP} PRIVATE /wd4324)
endif()

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
