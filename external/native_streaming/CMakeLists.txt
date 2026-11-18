opendaq_dependency(
    NAME                native_streaming
    REQUIRED_VERSION    1.0.21
    GIT_REPOSITORY      https://github.com/openDAQ/libNativeStreaming.git
    GIT_REF             1fd33a4ba1931f63630f064318a2002599972ea0
    EXPECT_TARGET       daq::native_streaming
)
