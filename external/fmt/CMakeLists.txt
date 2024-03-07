set(FMT_INSTALL ON CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                fmt
    REQUIRED_VERSION    10.2.1
    GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
    GIT_REF             10.2.1
    EXPECT_TARGET       fmt::fmt
)
