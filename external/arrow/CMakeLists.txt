set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ARROW_BUILD_SHARED ON)
set(ARROW_BUILD_STATIC OFF)
set(ARROW_BUILD_BENCHMARKS OFF)
set(ARROW_BUILD_EXAMPLES OFF)
set(ARROW_BUILD_TESTS OFF)
set(ARROW_ENABLE_TIMING_TESTS OFF)
set(ARROW_SIMD_LEVEL "NONE")
set(ARROW_RUNTIME_SIMD_LEVEL "NONE")
set(ARROW_PARQUET ON)
set(ARROW_POSITION_INDEPENDENT_CODE ON)
set(ARROW_IPC ON)

set(Thrift_VERSION 0.20.0)

set(PATCH_FILES ${CMAKE_CURRENT_SOURCE_DIR}/patches/001-fix-boost-linking.patch
                ${CMAKE_CURRENT_SOURCE_DIR}/patches/002-fix-boost-linking-thrift.patch)

opendaq_dependency(
    NAME                Arrow
    REQUIRED_VERSION    17.0.0
    GIT_REPOSITORY      https://github.com/apache/arrow
    GIT_REF             6a2e19a
    SOURCE_SUBDIR       cpp
    ADD_FETCH_ALIAS     Arrow::arrow_shared=arrow_shared
    EXPECT_TARGET       Arrow::arrow_shared
    PATCH_FILES         "${PATCH_FILES}"
)

if (Arrow_FETCHED)
    # Arrow was not designed to be used via FetchContent/add_subdirectory.
    # In this configuration it does not properly register its include paths
    # with the library target. We have to do it here manually.
    target_include_directories(arrow_shared INTERFACE
                                            $<BUILD_INTERFACE:${arrow_SOURCE_DIR}/cpp/src>
                                            $<BUILD_INTERFACE:${arrow_BINARY_DIR}/src>)
endif()
