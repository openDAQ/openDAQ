opendaq_dependency(
    NAME                native_streaming
    REQUIRED_VERSION    1.0.19
    GIT_REPOSITORY      https://github.com/openDAQ/libNativeStreaming.git
    GIT_REF             fix/fetching-boost
    EXPECT_TARGET       daq::native_streaming
)
