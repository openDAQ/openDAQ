set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.0
    GIT_REPOSITORY      https://github.com/hbkdaq/opendaq-ws-streaming
    GIT_REF             aa22bf36a5c11e9ca8f17b8fdd0c56bb1636582e
    EXPECT_TARGET       ws-streaming::ws-streaming
)
