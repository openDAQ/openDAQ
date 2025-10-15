set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.5
    GIT_REPOSITORY      https://github.com/openDAQ/ws-streaming
    GIT_REF             ad95ea62c946554c64370a8f4de6ba7a8d46ca49
    EXPECT_TARGET       ws-streaming::ws-streaming
)
