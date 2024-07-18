SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_BUILD_TYPE "Release")

SET(ARROW_BUILD_SHARED OFF)
SET(ARROW_BUILD_STATIC ON)
SET(ARROW_BUILD_BENCHMARKS OFF)
SET(ARROW_BUILD_EXAMPLES OFF)
SET(ARROW_BUILD_TESTS OFF)
SET(ARROW_ENABLE_TIMING_TESTS OFF)
SET(ARROW_SIMD_LEVEL "NONE")

#SET(ARROW_PARQUET ON) If on more error messages are available in CMakeLists.txt

opendaq_dependency(
    NAME                arrow
    REQUIRED_VERSION    17.0.0
    GIT_REPOSITORY      https://github.com/openDAQ/arrow
    GIT_REF             f2a1a1b
    EXPECT_TARGET       Arrow::arrow_static
)

# Archieved repository: Arrow would be better to use
#opendaq_dependency(
#    NAME                parquet-cpp
#    REQUIRED_VERSION    1.5.0
#    GIT_REPOSITORY      https://github.com/apache/parquet-cpp
#    GIT_REF             85a2af9
#    EXPECT_TARGET       parquet_static
#)