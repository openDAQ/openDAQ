opendaq_dependency(
    NAME                native_streaming
    REQUIRED_VERSION    1.0.25
    GIT_REPOSITORY      https://github.com/openDAQ/libNativeStreaming.git
    GIT_REF             108e5d929307f241e075821489286aa6e770decf
    EXPECT_TARGET       daq::native_streaming
)
