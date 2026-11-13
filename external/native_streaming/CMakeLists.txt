opendaq_dependency(
    NAME                native_streaming
    REQUIRED_VERSION    1.0.21
    GIT_REPOSITORY      https://github.com/dejancrn/libNativeStreaming.git
    GIT_REF             1b3f0fe2fb038e44554946feb27a43b6e86eea0a
    EXPECT_TARGET       daq::native_streaming
)
