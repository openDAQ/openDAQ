set(LIB_NAME ${SDK_TARGET_NAME}_native_streaming_protocol)

set(LIB_MAJOR_VERSION 1)
set(LIB_MINOR_VERSION 0)
set(LIB_PATCH_VERSION 0)

set(SRC_Cpp native_streaming_client_handler.cpp
            native_streaming_server_handler.cpp
            native_streaming_protocol_types.cpp
            server_session_handler.cpp
            client_session_handler.cpp
            base_session_handler.cpp
            subscribers_registry.cpp
)

set(SRC_PublicHeaders native_streaming_protocol.h
                      native_streaming_client_handler.h
                      native_streaming_server_handler.h
                      native_streaming_protocol_types.h
                      server_session_handler.h
                      client_session_handler.h
                      base_session_handler.h
                      subscribers_registry.h
)

set(INCLUDE_DIR ../include/native_streaming_protocol)
prepend_include(${INCLUDE_DIR} SRC_PublicHeaders)


set(SRC_PrivateHeaders )


add_library(${LIB_NAME} STATIC ${SRC_Cpp}
                               ${SRC_PublicHeaders}
                               ${SRC_PrivateHeaders}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if(BUILD_64Bit OR BUILD_ARM)
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

                                              $<INSTALL_INTERFACE:include>
)

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
                                         opendaq_packet_streaming
                                         daq::native_streaming
)

if (WIN32)
    target_link_libraries(${LIB_NAME} PUBLIC
            ws2_32
            wsock32
    )
endif()

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_output_lib_name(${LIB_NAME} ${LIB_MAJOR_VERSION})
