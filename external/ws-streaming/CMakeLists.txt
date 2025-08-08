set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.0
    GIT_REPOSITORY      https://github.com/hbkdaq/opendaq-ws-streaming
    GIT_REF             6163134c7d3eb31bc84e1b1baf0d51b13eccf5df
    EXPECT_TARGET       ws-streaming::ws-streaming
)
