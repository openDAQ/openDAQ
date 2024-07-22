set(BUILD_COMPILER OFF)
set(WITH_AS3 OFF CACHE BOOL "")
set(WITH_JAVA OFF CACHE BOOL "")
set(WITH_JAVASCRIPT OFF CACHE BOOL "")
set(WITH_NODEJS OFF CACHE BOOL "")
set(WITH_PYTHON OFF CACHE BOOL "")

opendaq_dependency(
    NAME                Thrift
    REQUIRED_VERSION    0.20.0
    GIT_REPOSITORY      https://github.com/apache/thrift
    GIT_REF             0d0ac75347267f7c12fc9b5b349bf9ef7f632347
    ADD_FETCH_ALIAS     thrift::thrift=thrift
    EXPECT_TARGET       thrift::thrift
)

if (Thrift_FETCHED)
    target_include_directories(thrift INTERFACE
        $<BUILD_INTERFACE:${thrift_SOURCE_DIR}/lib/cpp/src>
        $<BUILD_INTERFACE:${thrift_BINARY_DIR}>)
endif()
