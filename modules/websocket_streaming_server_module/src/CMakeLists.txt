# Windows NSIS package manager limits lenght of variables
# "openDAQ_[MODULE_NAME]_[SUFFIX]" with 60 characters max
# The suffix with max lenght is [Development_was_installed]
# so the shorter module name [ws_stream_srv_module] should be used instead of full name:
# [ws_stream_srv_module]
set(LIB_NAME ws_stream_srv_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                websocket_streaming_server_module_impl.h
                websocket_streaming_server_impl.h
)

set(SRC_Srcs module_dll.cpp
             websocket_streaming_server_module_impl.cpp
             websocket_streaming_server_impl.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/websocket_streaming_server_module_impl.h
                            ${MODULE_HEADERS_DIR}/websocket_streaming_server_impl.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            module_dll.cpp
                            websocket_streaming_server_module_impl.cpp
                            streaming_server_impl.cpp
)


add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME} PUBLIC  daq::opendaq
                                  PRIVATE daq::opendaq_websocket_streaming
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})

