set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.4
    GIT_REPOSITORY      https://github.com/openDAQ/ws-streaming
    GIT_REF             02d380defca70d84295ca52aec8ab0ee940b20bb
    EXPECT_TARGET       ws-streaming::ws-streaming
)
