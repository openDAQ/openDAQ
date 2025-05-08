set(STREAMING_PROTOCOL_ALWAYS_FETCH_DEPS ON CACHE BOOL "" FORCE)

if (OPENDAQ_ENABLE_WS_SIGGEN_INTEGRATION_TESTS)
    set(STREAMING_PROTOCOL_TOOLS ON CACHE BOOL "" FORCE)
endif()

opendaq_dependency(
    NAME                streaming_protocol
    REQUIRED_VERSION    1.2.4
    GIT_REPOSITORY      https://github.com/openDAQ/streaming-protocol-lt.git
    GIT_REF             3b56a7a066b059e7eb1692de5f6704815200a222
    EXPECT_TARGET       daq::streaming_protocol
)
