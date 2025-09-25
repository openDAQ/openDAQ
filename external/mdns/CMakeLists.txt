set(MDNS_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                mdns
    REQUIRED_VERSION    1.4.3
    GIT_REPOSITORY      https://github.com/mjansson/mdns.git
    GIT_REF             1.4.3
    EXPECT_TARGET       mdns::mdns
    PATCH_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0001-Extension.patch
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0002-Fix-warnings.patch
        ${CMAKE_CURRENT_SOURCE_DIR}/patches/0003-Fix-ipv6-socket-ifindex.patch
)
