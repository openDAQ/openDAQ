set(BASE_NAME websocket_streaming)
set(LIB_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_Cpp signal_descriptor_converter.cpp
            streaming_client.cpp
            streaming_server.cpp
            input_signal.cpp
            output_signal.cpp
            async_packet_reader.cpp
            websocket_streaming_server.cpp
            websocket_client_device_impl.cpp
            websocket_client_signal_impl.cpp
            websocket_streaming_impl.cpp
)

set(SRC_PublicHeaders
    websocket_streaming.h
    signal_info.h
    signal_descriptor_converter.h
    streaming_client.h
    streaming_server.h
    input_signal.h
    output_signal.h
    async_packet_reader.h
    websocket_streaming_server.h
    websocket_client_device_impl.h
    websocket_client_device_factory.h
    websocket_client_signal_impl.h
    websocket_client_signal_factory.h
    websocket_streaming_impl.h
    websocket_streaming_factory.h
)

set(INCLUDE_DIR ../include/websocket_streaming)
prepend_include(${INCLUDE_DIR} SRC_PublicHeaders)


set(SRC_PrivateHeaders
)


add_library(${LIB_NAME} STATIC ${SRC_Cpp}
                               ${SRC_PublicHeaders}
                               ${SRC_PrivateHeaders}
)

add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${LIB_NAME})

if(BUILD_64Bit OR BUILD_ARM)
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

                                              $<INSTALL_INTERFACE:include>
)

# links target with daq::streaming_dev (which is a static lib)
# because daq::streaming_dev is also linked into opcuatms_client lib
# and both target & opcuatms_client are lately linked into integration test binary,
# to avoid multiple definition linker errors both target & opcuatms_client should use daq::streaming_dev
target_link_libraries(${LIB_NAME} PUBLIC daq::streaming_protocol
                                         daq::opendaq
                                  PRIVATE
                                         daq::streaming_dev
)

# Fix daq::streaming_protocol` not properly propagating linking requirements on Windows
if (WIN32)
    target_link_libraries(${LIB_NAME}
        PUBLIC
            ws2_32
            wsock32
    )
endif()

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_PublicHeaders}")

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
