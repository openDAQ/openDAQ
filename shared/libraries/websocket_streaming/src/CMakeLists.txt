set(BASE_NAME websocket_streaming)

set(LIB_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_Cpp
    descriptor_to_metadata.cpp
    metadata_to_descriptor.cpp
    remote_signal_handler.cpp
    ws_streaming_device.cpp
    ws_streaming_listener.cpp
    ws_streaming_server.cpp
    ws_streaming_signal.cpp
    ws_streaming.cpp
)

set(SRC_PublicHeaders
    common.h
    descriptor_to_metadata.h
    metadata_to_descriptor.h
    remote_signal_handler.h
    ws_streaming_device.h
    ws_streaming_listener.h
    ws_streaming_remote_signal_entry.h
    ws_streaming_server.h
    ws_streaming_signal.h
    ws_streaming.h
)

set(INCLUDE_DIR ../include/websocket_streaming)
prepend_include(${INCLUDE_DIR} SRC_PublicHeaders)

add_library(${LIB_NAME} STATIC ${SRC_Cpp} ${SRC_PublicHeaders})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${LIB_NAME})

if(BUILD_64Bit OR BUILD_ARM)
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()

target_include_directories(${LIB_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::opendaq
        ws-streaming::ws-streaming
)

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
