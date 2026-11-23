opendaq_dependency(
    NAME                native_streaming
    REQUIRED_VERSION    1.0.23
    GIT_REPOSITORY      https://github.com/openDAQ/libNativeStreaming.git
    GIT_REF             340962a95c73672fd15ce3b675c6447eb8daa335
    EXPECT_TARGET       daq::native_streaming
)
