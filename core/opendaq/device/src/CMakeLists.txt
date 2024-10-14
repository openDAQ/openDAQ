set(BASE_NAME device)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Device device.h)
    rtgen(SRC_DevicePrivate device_private.h)
    rtgen(SRC_DeviceDomain device_domain.h)
    rtgen(SRC_DeviceInfo device_info.h)
    rtgen(SRC_DeviceInfoInternal device_info_internal.h)
    rtgen(SRC_DeviceInfoConfig device_info_config.h)
    rtgen(SRC_ServerCapability server_capability.h)
    rtgen(SRC_ServerCapabilityConfig server_capability_config.h)
    rtgen(SRC_IoFolderConfig io_folder_config.h)
    rtgen(SRC_DeviceType device_type.h)
    rtgen(SRC_AddressInfo address_info.h)
    rtgen(SRC_AddressInfoBuilder address_info_builder.h)
    rtgen(SRC_AddressInfoPrivate address_info_private.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_Device_PublicHeaders}
        ${SRC_DeviceDomain_PublicHeaders}
        ${SRC_DeviceInfo_PublicHeaders}
        ${SRC_DeviceInfoConfig_PublicHeaders}
        ${SRC_DeviceInfoInternal_PublicHeaders}
        ${SRC_ServerCapability_PublicHeaders}
        ${SRC_ServerCapabilityConfig_PublicHeaders}
        ${SRC_IoFolderConfig_PublicHeaders}
        ${SRC_DeviceType_PublicHeaders}
        ${SRC_AddressInfo_PublicHeaders}
        ${SRC_AddressInfoBuilder_PublicHeaders}
        ${SRC_AddressInfoPrivate_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_Device_PrivateHeaders}
        ${SRC_DeviceDomain_PrivateHeaders}
        ${SRC_DeviceInfo_PrivateHeaders}
        ${SRC_DeviceInfoConfig_PrivateHeaders}
        ${SRC_DeviceInfoInternal_PrivateHeaders}
        ${SRC_ServerCapability_PrivateHeaders}
        ${SRC_ServerCapabilityConfig_PrivateHeaders}
        ${SRC_IoFolderConfig_PrivateHeaders}
        ${SRC_DeviceType_PrivateHeaders}
        ${SRC_AddressInfo_PrivateHeaders}
        ${SRC_AddressInfoBuilder_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Device_Cpp}
        ${SRC_DeviceDomain_Cpp}
        ${SRC_DeviceInfo_Cpp}
        ${SRC_DeviceInfoConfig_Cpp}
        ${SRC_DeviceInfoInternal_Cpp}
        ${SRC_ServerCapability_Cpp}
        ${SRC_ServerCapabilityConfig_Cpp}
        ${SRC_IoFolderConfig_Cpp}
        ${SRC_DeviceType_Cpp}
        ${SRC_AddressInfo_Cpp}
        ${SRC_AddressInfoBuilder_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "device/include/opendaq")
    set(SDK_SRC_DIR "device/src")
    
    source_group("device//device" FILES 
        ${SDK_HEADERS_DIR}/device.h
        ${SDK_HEADERS_DIR}/device_impl.h
        ${SDK_HEADERS_DIR}/device_domain.h
        ${SDK_HEADERS_DIR}/device_info.h
        ${SDK_HEADERS_DIR}/device_info_config.h
        ${SDK_HEADERS_DIR}/device_info_internal.h
        ${SDK_HEADERS_DIR}/device_info_impl.h
        ${SDK_HEADERS_DIR}/device_info_factory.h
        ${SDK_HEADERS_DIR}/device_private.h
        ${SDK_SRC_DIR}/device_info_impl.cpp
    )
    
    source_group("device//device_domain" FILES 
        ${SDK_HEADERS_DIR}/device_domain.h
        ${SDK_HEADERS_DIR}/device_domain_impl.h
        ${SDK_HEADERS_DIR}/device_domain_factory.h
        ${SDK_SRC_DIR}/device_domain_impl.cpp
    )
    
    source_group("device//device_type" FILES 
        ${SDK_HEADERS_DIR}/device_type.h
        ${SDK_HEADERS_DIR}/device_type_impl.h
        ${SDK_HEADERS_DIR}/device_type_factory.h
        ${SDK_SRC_DIR}/device_type_impl.cpp
    )
    
    source_group("device//io_folder" FILES 
        ${SDK_HEADERS_DIR}/io_folder_config.h
        ${SDK_HEADERS_DIR}/io_folder_impl.h
        ${SDK_HEADERS_DIR}/io_folder_factory.h
        ${SDK_SRC_DIR}/io_folder_impl.cpp
    )
    
    source_group("device//core_events" FILES 
        ${SDK_HEADERS_DIR}/core_opendaq_event_args.h
        ${SDK_HEADERS_DIR}/core_opendaq_event_args_factory.h
        ${SDK_HEADERS_DIR}/core_opendaq_event_args_impl.h
        ${SDK_SRC_DIR}/core_opendaq_event_args_impl.cpp
    )
    
    source_group("device//server_capability" FILES 
        ${SDK_HEADERS_DIR}/server_capability.h
        ${SDK_HEADERS_DIR}/server_capability_config.h
        ${SDK_HEADERS_DIR}/server_capability_impl.h
        ${SDK_HEADERS_DIR}/address_info.h
        ${SDK_HEADERS_DIR}/address_info_private.h
        ${SDK_HEADERS_DIR}/address_info_builder.h
        ${SDK_HEADERS_DIR}/address_info_impl.h
        ${SDK_HEADERS_DIR}/address_info_builder_impl.h
        ${SDK_SRC_DIR}/address_info_impl.cpp
        ${SDK_SRC_DIR}/address_info_builder_impl.cpp
    )
    
    source_group("device" FILES 
        ${SDK_HEADERS_DIR}/device_errors.h
        ${SDK_HEADERS_DIR}/device_exceptions.h
        ${SDK_SRC_DIR}/device.natvis
    )
endfunction()

set(SRC_PublicHeaders_Component 
    device_info_factory.h
    device_errors.h
    device_exceptions.h
    device_impl.h
    io_folder_factory.h
    io_folder_impl.h
    device_type_factory.h
    device_private.h
    core_opendaq_event_args_factory.h
    core_opendaq_event_args.h
    server_capability.h
    server_capability_config.h
    server_capability_impl.h
    device_domain_factory.h
    address_info_factory.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    device_info_impl.h
    device_type_impl.h
    device_domain_impl.h
    address_info_impl.h
    address_info_builder_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    device_info_impl.cpp
    io_folder_impl.cpp
    device_type_impl.cpp
    core_opendaq_event_args_impl.cpp
    server_capability_impl.cpp
    device_domain_impl.cpp
    address_info_impl.cpp
    address_info_builder_impl.cpp
    device.natvis
    PARENT_SCOPE
)
