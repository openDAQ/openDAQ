option(MI_OVERRIDE OFF)
set(MIMALLOC_INSTALL ON CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                mimalloc
    REQUIRED_VERSION    2.1.1
    GIT_REPOSITORY      https://github.com/microsoft/mimalloc.git
    GIT_REF             2.1.1
    EXPECT_TARGET       mimalloc-static
)
