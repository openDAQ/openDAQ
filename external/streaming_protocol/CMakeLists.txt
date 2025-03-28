set(STREAMING_PROTOCOL_ALWAYS_FETCH_DEPS ON CACHE BOOL "" FORCE)

if (OPENDAQ_ENABLE_WS_SIGGEN_INTEGRATION_TESTS)
    set(STREAMING_PROTOCOL_TOOLS ON CACHE BOOL "" FORCE)
endif()

opendaq_dependency(
    NAME                streaming_protocol
    REQUIRED_VERSION    1.2.2
    GIT_REPOSITORY      https://github.com/openDAQ/streaming-protocol-lt.git
    GIT_REF             eb540691da1089a61de1d297480cd5fd2ffc3fe9
    EXPECT_TARGET       daq::streaming_protocol
)
