set(FMT_INSTALL ON CACHE BOOL "" FORCE)

if (CMAKE_COMPILER_IS_GNUCXX
    AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 10.0
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS    12.0
)
    set(FMT_SYSTEM_HEADERS ON CACHE BOOL "Expose headers with marking them as system." FORCE)
endif()

opendaq_dependency(
    NAME                fmt
    REQUIRED_VERSION    10.2.1
    GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
    GIT_REF             10.2.1
    EXPECT_TARGET       fmt::fmt
)
