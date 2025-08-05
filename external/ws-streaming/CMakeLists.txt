set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.3
    GIT_REPOSITORY      https://github.com/openDAQ/ws-streaming
    GIT_REF             d89ebf8ccfc1f6e6767a6fa2a3979b6aa9a59c31
    EXPECT_TARGET       ws-streaming::ws-streaming
)
