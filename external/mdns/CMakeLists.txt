set(MDNS_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                mdns
    REQUIRED_VERSION    1.4.2
    GIT_REPOSITORY      https://github.com/mjansson/mdns.git
    GIT_REF             1.4.3
    EXPECT_TARGET       mdns::mdns
)
