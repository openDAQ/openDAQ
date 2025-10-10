set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.5
    GIT_REPOSITORY      https://github.com/openDAQ/ws-streaming
    GIT_REF             f903260cf154ea0cd36bcb692cc635737e16501e
    EXPECT_TARGET       ws-streaming::ws-streaming
)
