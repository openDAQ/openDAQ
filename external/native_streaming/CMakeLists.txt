opendaq_dependency(
    NAME                native_streaming
    REQUIRED_VERSION    1.0.22
    GIT_REPOSITORY      https://github.com/dejancrn/libNativeStreaming.git
    GIT_REF             e76ac675b932f0f15577dae6cc0687807a68f028
    EXPECT_TARGET       daq::native_streaming
)
