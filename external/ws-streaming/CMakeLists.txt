set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.6
    GIT_REPOSITORY      https://github.com/openDAQ/ws-streaming
    GIT_REF             169cc3198f69325f2fe5fc1bd59e938074600c66
    EXPECT_TARGET       ws-streaming::ws-streaming
)
