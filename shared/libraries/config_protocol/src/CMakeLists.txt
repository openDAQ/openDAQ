set(BASE_NAME config_protocol)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})

set(CONFIG_HEADERS_DIR ../include/${BASE_NAME})

set(RTGEN_LIBRARY_NAME ${BASE_NAME})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${CONFIG_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(RTGEN_RELATIVE_PARENT_DIR shared/libraries/${BASE_NAME})

rtgen(SRC_ConfigClientObject config_client_object.h)

set(SRC_PublicHeaders config_protocol.h
                      config_protocol_server.h
                      config_protocol_client.h
                      server_wrappers.h
                      config_client_object_impl.h
                      config_client_property_object_impl.h
                      config_client_component_impl.h
                      config_client_folder_impl.h
                      config_client_device_impl.h
                      config_client_io_folder_impl.h
                      config_client_function_block_impl.h
                      config_client_channel_impl.h
                      config_client_signal_impl.h
                      config_client_input_port_impl.h
                      config_client_function_impl.h
                      config_client_procedure_impl.h                      
                      config_client_connection_impl.h
                      config_client_input_port.h
                      config_server_component.h
                      config_server_device.h
                      config_server_signal.h
                      config_protocol_deserialize_context.h
)

set(SRC_PrivateHeaders config_protocol_deserialize_context_impl.h
                       config_server_input_port.h
)                       


set(SRC_Cpp config_protocol.cpp
            config_protocol_server.cpp
            config_protocol_client.cpp
            server_wrappers.cpp
            config_client_object_impl.cpp
            config_protocol_deserialize_context_impl.cpp
)

prepend_include(${BASE_NAME} SRC_PublicHeaders)
prepend_include(${BASE_NAME} SRC_PrivateHeaders)

list(APPEND SRC_PublicHeaders ${SRC_ConfigClientObject_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_ConfigClientObject_PrivateHeaders}
)

add_library(${MODULE_NAME} STATIC ${SRC_HEADERS} ${SRC_Cpp} ${SRC_PublicHeaders} ${SRC_PrivateHeaders})
add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${MODULE_NAME})

if (MSVC)
    target_compile_options(${MODULE_NAME} PRIVATE /bigobj)
elseif (MINGW AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${MODULE_NAME} PRIVATE -Wa,-mbig-obj)
endif()

target_include_directories(${MODULE_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/../include)

target_link_libraries(${MODULE_NAME} 
    PUBLIC 
        daq::opendaq
)
