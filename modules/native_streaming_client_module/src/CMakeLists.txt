# Windows NSIS package manager limits lenght of variables
# "openDAQ_[MODULE_NAME]_[SUFFIX]" with 60 characters max
# The suffix with max lenght is [Development_was_installed]
# so the shorter module name [native_stream_cl_module] should be used instead of full name:
# [native_streaming_client_module]
set(LIB_NAME native_stream_cl_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                native_streaming_client_module_impl.h
                native_streaming_device_impl.h
                native_streaming_signal_impl.h
                native_streaming_impl.h
                native_device_impl.h
)

set(SRC_Srcs module_dll.cpp
             native_streaming_client_module_impl.cpp
             native_streaming_device_impl.cpp
             native_streaming_signal_impl.cpp
             native_streaming_impl.cpp
             native_device_impl.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/native_streaming_client_module_impl.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            native_streaming_client_module_impl.cpp
)

source_group("device" FILES ${MODULE_HEADERS_DIR}/native_streaming_device_impl.h
                            ${MODULE_HEADERS_DIR}/native_device_impl.h
                            native_streaming_device_impl.cpp
                            native_device_impl.cpp
)

source_group("signal" FILES ${MODULE_HEADERS_DIR}/native_streaming_signal_impl.h
                            native_streaming_signal_impl.cpp
)

source_group("streaming" FILES ${MODULE_HEADERS_DIR}/native_streaming_impl.h
                               native_streaming_impl.cpp
)

add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

if (WIN32)
    set(BCRYPT_LIB bcrypt.dll)
endif()

target_link_libraries(${LIB_NAME} PUBLIC  daq::opendaq
                                  PRIVATE daq::discovery
                                          daq::native_streaming_protocol
                                          daq::config_protocol
                                          daq::opendaq_dev
                                          ${BCRYPT_LIB}
                                          $<BUILD_INTERFACE:Boost::uuid>
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
                                              ${Boost_INCLUDE_DIRS}
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
