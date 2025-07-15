if(MSVC)
    remove_definitions(-D_UNICODE -DUNICODE)
    include_directories($<TARGET_PROPERTY:Boost::multiprecision,INTERFACE_INCLUDE_DIRECTORIES>)
    include_directories($<TARGET_PROPERTY:Boost::predef,INTERFACE_INCLUDE_DIRECTORIES>)
    include_directories($<TARGET_PROPERTY:Boost::scope_exit,INTERFACE_INCLUDE_DIRECTORIES>)
    include_directories($<TARGET_PROPERTY:Boost::typeof,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

set(ARROW_MIMALLOC OFF CACHE BOOL "")
set(ARROW_WITH_RE2 OFF CACHE BOOL "")
set(ARROW_WITH_UTF8PROC OFF CACHE BOOL "")
set(ARROW_PARQUET ON CACHE BOOL "")
set(ARROW_DEFINE_OPTIONS ON CACHE BOOL "")
set(ARROW_SIMD_LEVEL NONE CACHE STRING "")
set(ARROW_RUNTIME_SIMD_LEVEL NONE CACHE STRING "")
set(ARROW_IPC ON CACHE BOOL "")
set(ARROW_VERBOSE_THIRDPARTY_BUILD ON CACHE BOOL "")
set(ARROW_DEPENDENCY_USE_SHARED OFF CACHE BOOL "")
set(ARROW_BOOST_USE_SHARED OFF CACHE BOOL "")

set(ARROW_POSITION_INDEPENDENT_CODE OFF CACHE BOOL "")
set(ARROW_BUILD_SHARED OFF CACHE BOOL "")
set(ARROW_BUILD_STATIC ON CACHE BOOL "")
set(ARROW_BUILD_BENCHMARKS OFF CACHE BOOL "")
set(ARROW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(ARROW_BUILD_TESTS OFF CACHE BOOL "")
set(ARROW_ENABLE_TIMING_TESTS OFF CACHE BOOL "")

set(ARROW_BUILD_TESTS OFF CACHE BOOL "")

set(PARQUET_MINIMAL_DEPENDENCY ON CACHE BOOL "")
set(ARROW_BUILD_TESTS OFF CACHE BOOL "")
set(Thrift_SOURCE AUTO CACHE STRING "")
set(Thrift_VERSION 0.20.0 CACHE STRING "")
set(ARROW_OPTIONAL_INSTALL OFF CACHE BOOL "")

if(UNIX AND NOT APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

opendaq_dependency(
    NAME                Arrow
    REQUIRED_VERSION    20.0.0
    GIT_REPOSITORY      https://github.com/apache/arrow.git
    GIT_REF             apache-arrow-20.0.0
    ADD_FETCH_ALIAS     Arrow::arrow_static=arrow_static
    EXPECT_TARGET       Arrow::arrow_static
    SOURCE_SUBDIR       cpp
    PATCH_FILES        
                        "${CMAKE_CURRENT_SOURCE_DIR}/boost_linking.patch"
)

if (Arrow_FETCHED)
    target_link_libraries(parquet_static PUBLIC thrift::thrift)
    target_include_directories(parquet_static INTERFACE
        "$<TARGET_PROPERTY:thrift::thrift,INTERFACE_INCLUDE_DIRECTORIES>")
    target_include_directories(arrow_static INTERFACE
        $<BUILD_INTERFACE:${arrow_SOURCE_DIR}/cpp/src>
        $<BUILD_INTERFACE:${arrow_BINARY_DIR}/src>)
endif()