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
set(ARROW_PARQUET ON)

set(Thrift_VERSION 0.20.0)

opendaq_dependency(
    NAME                Arrow
    REQUIRED_VERSION    18.0.0
    GIT_REPOSITORY      https://github.com/openDAQ/arrow
    GIT_REF             bd9923826341065780bdf5acc9c74c346050996c
    SOURCE_SUBDIR       cpp
    ADD_FETCH_ALIAS     Arrow::arrow_static=arrow_static
    EXPECT_TARGET       Arrow::arrow_static
)
