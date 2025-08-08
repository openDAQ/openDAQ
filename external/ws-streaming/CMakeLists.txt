set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.0
    GIT_REPOSITORY      https://github.com/hbkdaq/opendaq-ws-streaming
    GIT_REF             5f38cda72d89ee75e2ec480a4192526677ca855d
    EXPECT_TARGET       ws-streaming::ws-streaming
)
