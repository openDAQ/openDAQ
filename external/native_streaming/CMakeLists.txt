opendaq_dependency(
    NAME                native_streaming
    REQUIRED_VERSION    1.0.24
    GIT_REPOSITORY      https://github.com/openDAQ/libNativeStreaming.git
    GIT_REF             01f4c5189183b9893a6a89cf1e5bd19e2f461325
    EXPECT_TARGET       daq::native_streaming
)
