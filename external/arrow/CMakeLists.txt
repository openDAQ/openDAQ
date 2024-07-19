set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Release")

set(ARROW_BUILD_SHARED OFF)
set(ARROW_BUILD_STATIC ON)
set(ARROW_BUILD_BENCHMARKS OFF)
set(ARROW_BUILD_EXAMPLES OFF)
set(ARROW_BUILD_TESTS OFF)
set(ARROW_ENABLE_TIMING_TESTS OFF)
set(ARROW_SIMD_LEVEL "NONE")
set(ARROW_RUNTIME_SIMD_LEVEL "NONE")

opendaq_dependency(
    NAME                Arrow
    REQUIRED_VERSION    18.0.0
    GIT_REPOSITORY      https://github.com/apache/arrow
    GIT_REF             299ad7086928c555f3e861c20327276d1c4f2557
    SOURCE_SUBDIR       cpp
    ADD_FETCH_ALIAS     Arrow::arrow_static=arrow_static
    EXPECT_TARGET       Arrow::arrow_static
)
