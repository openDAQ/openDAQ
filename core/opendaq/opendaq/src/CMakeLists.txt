set(BASE_NAME opendaq)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Instance instance.h)
    rtgen(SRC_InstanceBuilder instance_builder.h)
    rtgen(SRC_ConfigProvider config_provider.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${ConfigHeader}
        ${SRC_Instance_PublicHeaders}
        ${SRC_InstanceBuilder_PublicHeaders}
        ${SRC_ConfigProvider_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_Instance_PrivateHeaders}
        ${SRC_InstanceBuilder_PrivateHeaders}
        ${SRC_ConfigProvider_PrivateHeaders}
        ${SRC_ComponentTypeBuilder_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Instance_Cpp}
        ${SRC_InstanceBuilder_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "opendaq/include/opendaq")
    set(SDK_SRC_DIR "opendaq/src")
    
    source_group("opendaq//instance" FILES 
        ${SDK_HEADERS_DIR}/instance.h
        ${SDK_HEADERS_DIR}/instance_impl.h
        ${SDK_HEADERS_DIR}/instance_factory.h
        ${SDK_HEADERS_DIR}/client_impl.h
        ${SDK_HEADERS_DIR}/instance_builder.h
        ${SDK_HEADERS_DIR}/instance_builder_impl.h
        ${SDK_HEADERS_DIR}/config_provider.h
        ${SDK_HEADERS_DIR}/config_provider_factory.h
        ${SDK_HEADERS_DIR}/path_tool.h
        ${SDK_HEADERS_DIR}/json_config_provider_impl.h
        ${SDK_HEADERS_DIR}/base_config_provider_impl.h
        ${SDK_HEADERS_DIR}/env_config_provider_impl.h
        ${SDK_HEADERS_DIR}/cmd_line_args_config_provider_impl.h
        ${SDK_SRC_DIR}/instance_impl.cpp
        ${SDK_SRC_DIR}/client_impl.cpp
        ${SDK_SRC_DIR}/instance_builder_impl.cpp
        ${SDK_SRC_DIR}/json_config_provider_impl.cpp
        ${SDK_SRC_DIR}/base_config_provider_impl.cpp
        ${SDK_SRC_DIR}/env_config_provider_impl.cpp
        ${SDK_SRC_DIR}/cmd_line_args_config_provider_impl.cpp
        ${SDK_SRC_DIR}/path_tool.cpp             
    )
    
    source_group("component_type" FILES 
        ${SDK_HEADERS_DIR}/component_type_builder_impl.h
        component_type_builder_impl.cpp
    )
endfunction()

set(ConfigHeaderSource ../include/opendaq/opendaq_config.h.in)
set(ConfigHeader ${CMAKE_CURRENT_BINARY_DIR}/../include/${MAIN_TARGET}/opendaq_config.h)
configure_file(${ConfigHeaderSource} ${ConfigHeader})


set(SRC_PublicHeaders_Component 
    version.h
    opendaq.h
    errors.h
    exceptions.h
    instance_factory.h
    config_provider_factory.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    instance_impl.h
    client_impl.h
    opendaq_init.h
    instance_builder_impl.h
    path_tool.h
    json_config_provider_impl.h
    base_config_provider_impl.h
    env_config_provider_impl.h
    cmd_line_args_config_provider_impl.h
    component_type_builder_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    version.cpp
    instance_impl.cpp
    client_impl.cpp
    opendaq_init.cpp
    instance_builder_impl.cpp
    path_tool.cpp
    json_config_provider_impl.cpp
    base_config_provider_impl.cpp
    env_config_provider_impl.cpp
    cmd_line_args_config_provider_impl.cpp
    component_type_builder_impl.cpp
    opendaq.natvis
    PARENT_SCOPE
)

set(SRC_PrivateLinkLibraries_Component
    $<BUILD_INTERFACE:Boost::uuid>
    $<BUILD_INTERFACE:Boost::algorithm>
    $<BUILD_INTERFACE:rapidjson>
    PARENT_SCOPE
)

set_source_files_properties(${ConfigHeader} PROPERTIES GENERATED TRUE)
