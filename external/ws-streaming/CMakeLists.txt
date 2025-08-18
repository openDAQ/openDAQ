set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.0
    GIT_REPOSITORY      https://github.com/openDAQ/ws-streaming
    GIT_REF             523357bd4790ab5d92c326bc4bb750b2de841db0
    EXPECT_TARGET       ws-streaming::ws-streaming
)
