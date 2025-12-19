option(MI_OVERRIDE OFF)
set(MIMALLOC_INSTALL ON CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                mimalloc
    REQUIRED_VERSION    3.0.11
    GIT_REPOSITORY      https://github.com/microsoft/mimalloc.git
    GIT_REF             v3.0.11
    EXPECT_TARGET       mimalloc-static
)
