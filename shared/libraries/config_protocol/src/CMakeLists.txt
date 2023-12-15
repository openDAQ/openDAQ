set(BASE_NAME config_protocol)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(SRC_HEADERS config_protocol.h
                config_protocol_server.h
                config_protocol_client.h
                server_wrappers.h
                config_client_object_impl.h
                config_client_property_object_impl.h
                config_client_component_impl.h
                config_client_folder_impl.h
                config_client_device_impl.h
)

set(SRC_CPPS config_protocol.cpp
             config_protocol_server.cpp
             config_protocol_client.cpp
             server_wrappers.cpp
             config_client_object_impl.cpp
)

prepend_include(config_protocol SRC_HEADERS)

add_library(${MODULE_NAME} STATIC ${SRC_HEADERS} ${SRC_CPPS})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(${MODULE_NAME} 
    PUBLIC 
        daq::opendaq
    PRIVATE
        daq::utility)
