set(FMT_INSTALL ON CACHE BOOL "" FORCE)

opendaq_dependency(
    NAME                fmt
    REQUIRED_VERSION    7.0.3
    GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
    GIT_REF             7.0.3
    EXPECT_TARGET       fmt::fmt
)
