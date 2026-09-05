opendaq_dependency(
    NAME                native_streaming
    REQUIRED_VERSION    1.0.19
    GIT_REPOSITORY      https://github.com/openDAQ/libNativeStreaming.git
    GIT_REF             be4c444cd8ed2dd87d10747fc961f015893141fd
    # A commit pin needs the full history, unlike the tag it replaces.
    GIT_SHALLOW         OFF
    EXPECT_TARGET       daq::native_streaming
)
