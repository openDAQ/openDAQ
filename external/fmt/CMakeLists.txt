set(FMT_INSTALL ON CACHE BOOL "" FORCE)
set(FMT_WERROR OFF CACHE BOOL "" FORCE)

if ((CMAKE_COMPILER_IS_GNUCXX
    AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 10.0
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS    12.0)
    OR MSVC
)
    set(FMT_SYSTEM_HEADERS ON CACHE BOOL "Expose headers with marking them as system." FORCE)
endif()

opendaq_dependency(
    NAME                fmt
    REQUIRED_VERSION    12.1.0
    GIT_REPOSITORY      https://github.com/fmtlib/fmt.git
    GIT_REF             02d6f3d9e420e545509f49b5d827911a3c9690fc # v12.1.1
    GIT_SHALLOW       OFF
    EXPECT_TARGET       fmt::fmt
)
