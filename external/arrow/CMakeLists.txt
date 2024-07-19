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

set(PATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}/patches/001-fix-boost-linking.patch)

opendaq_dependency(
    NAME                Arrow
    REQUIRED_VERSION    17.0.0
    GIT_REPOSITORY      https://github.com/apache/arrow
    GIT_REF             6a2e19a
    SOURCE_SUBDIR       cpp
    ADD_FETCH_ALIAS     Arrow::arrow_static=arrow_static
    EXPECT_TARGET       Arrow::arrow_static
    PATCH_FILES         "${PATCH_FILES}"
)
