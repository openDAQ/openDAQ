set(WS_STREAMING_INSTALL OFF)

opendaq_dependency(
    NAME                ws-streaming
    REQUIRED_VERSION    3.0.7
    GIT_REPOSITORY      https://github.com/openDAQ/ws-streaming
    GIT_REF             9d591b39ec4845a9f65c558411d809800d455a5d
    EXPECT_TARGET       ws-streaming::ws-streaming
    PATCH_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0001-Fixes-plus.patch
)
