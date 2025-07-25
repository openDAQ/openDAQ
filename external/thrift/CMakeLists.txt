set(BUILD_COMPILER OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
set(BUILD_TUTORIALS OFF CACHE BOOL "")
set(WITH_AS3 OFF CACHE BOOL "")
set(WITH_CPP ON CACHE BOOL "")
set(WITH_C_GLIB OFF CACHE BOOL "")
set(WITH_JAVA OFF CACHE BOOL "")
set(WITH_JAVASCRIPT OFF CACHE BOOL "")
set(WITH_LIBEVENT OFF CACHE BOOL "")
set(WITH_NODEJS OFF CACHE BOOL "")
set(WITH_PYTHON OFF CACHE BOOL "")
set(WITH_QT5 OFF CACHE BOOL "")
set(WITH_ZLIB OFF CACHE BOOL "")
set(WITH_OPENSSL OFF CACHE BOOL "")

opendaq_dependency(
    NAME                Thrift
    REQUIRED_VERSION    0.20.0
    GIT_REPOSITORY      https://github.com/apache/thrift
    GIT_REF             v0.20.0
    ADD_FETCH_ALIAS     thrift::thrift=thrift
    EXPECT_TARGET       thrift::thrift
)

if (Thrift_FETCHED)

    target_link_libraries(thrift PUBLIC daq::boost_headers)

    target_include_directories(thrift INTERFACE
        "$<BUILD_INTERFACE:${thrift_SOURCE_DIR}/lib/cpp/src>"
        "$<BUILD_INTERFACE:${thrift_BINARY_DIR}>")

    get_target_property(thrift_include_dirs thrift INTERFACE_INCLUDE_DIRECTORIES)

endif()