set(BASE_NAME packet_streaming)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_HEADERS packet_streaming.h
                packet_streaming_server.h
                packet_streaming_client.h
)

set(SRC_CPPS packet_streaming.cpp
             packet_streaming_server.cpp
             packet_streaming_client.cpp
)

prepend_include(packet_streaming SRC_HEADERS)

add_library(${MODULE_NAME} STATIC ${SRC_HEADERS} ${SRC_CPPS})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(${MODULE_NAME} PUBLIC daq::opendaq)
