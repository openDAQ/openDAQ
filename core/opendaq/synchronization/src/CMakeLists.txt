set(BASE_NAME synchronization)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_SyncComponent sync_component.h)
    rtgen(SRC_SyncComponentPrivate sync_component_private.h)
    rtgen(SRC_SyncInterface sync_interface.h)
    rtgen(SRC_SyncInterfaceInternal sync_interface_internal.h INTERNAL)
    rtgen(SRC_SyncComponent2 sync_component2.h)
    rtgen(SRC_SyncComponent2Internal sync_component2_internal.h INTERNAL)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_SyncComponent_PublicHeaders}
        ${SRC_SyncComponentPrivate_PublicHeaders}
        ${SRC_SyncInterface_PublicHeaders}
        ${SRC_SyncInterfaceInternal_PublicHeaders}
        ${SRC_SyncComponent2_PublicHeaders}
        ${SRC_SyncComponent2Internal_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_SyncComponent_PrivateHeaders}
        ${SRC_SyncComponentPrivate_PrivateHeaders}
        ${SRC_SyncInterface_PrivateHeaders}
        ${SRC_SyncInterfaceInternal_PrivateHeaders}
        ${SRC_SyncComponent2_PrivateHeaders}
        ${SRC_SyncComponent2Internal_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_SyncComponent_Cpp}
        ${SRC_SyncComponentPrivate_Cpp}
        ${SRC_SyncInterface_Cpp}
        ${SRC_SyncInterfaceInternal_Cpp}
        ${SRC_SyncComponent2_Cpp}
        ${SRC_SyncComponent2Internal_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "synchronization/include/opendaq")
    set(SDK_SRC_DIR "synchronization/src")
    
    source_group("synchronization//synchronization" FILES 
        ${SDK_HEADERS_DIR}/sync_component.h
        ${SDK_HEADERS_DIR}/sync_component_private.h
        ${SDK_HEADERS_DIR}/sync_component_impl.h
        ${SDK_HEADERS_DIR}/sync_component_factory.h
        ${SDK_HEADERS_DIR}/sync_interface.h
        ${SDK_HEADERS_DIR}/sync_interface_internal.h
        ${SDK_HEADERS_DIR}/sync_component2.h
        ${SDK_HEADERS_DIR}/sync_component2_internal.h
        ${SDK_HEADERS_DIR}/sync_component2_impl.h
        ${SDK_HEADERS_DIR}/sync_interface_base_impl.h
        ${SDK_HEADERS_DIR}/ptp_sync_interface_base.h
        ${SDK_HEADERS_DIR}/clock_sync_interface_impl.h
        ${SDK_SRC_DIR}/sync_component_impl.cpp
        ${SDK_SRC_DIR}/sync_component2_impl.cpp
        ${SDK_SRC_DIR}/sync_interface_base_impl.cpp
        ${SDK_SRC_DIR}/ptp_sync_interface_base.cpp
        ${SDK_SRC_DIR}/clock_sync_interface_impl.cpp
    )
    
  
endfunction()

set(SRC_PublicHeaders_Component 
    sync_component.h
    sync_component_private.h
    sync_component_factory.h
    sync_interface.h
    sync_interface_internal.h
    sync_component2.h
    sync_component2_internal.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component 
    sync_component_impl.h
    sync_component2_impl.h
    sync_interface_base_impl.h
    ptp_sync_interface_base.h
    clock_sync_interface_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    sync_component_impl.cpp
    sync_component2_impl.cpp
    sync_interface_base_impl.cpp
    ptp_sync_interface_base.cpp
    clock_sync_interface_impl.cpp
    PARENT_SCOPE
)
