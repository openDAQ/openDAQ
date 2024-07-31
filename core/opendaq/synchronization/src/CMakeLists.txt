set(BASE_NAME synchronization)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_SyncComponent sync_component.h)
    rtgen(SRC_SyncComponentInternal sync_component_internal.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_SyncComponent_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_SyncComponent_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_SyncComponent_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "synchronization/include/opendaq")
    set(SDK_SRC_DIR "synchronization/src")
    
    source_group("synchronization//synchronization" FILES 
        ${SDK_HEADERS_DIR}/sync_component.h
        ${SDK_HEADERS_DIR}/sync_component_internal.h
        ${SDK_HEADERS_DIR}/sync_component_impl.h
        ${SDK_HEADERS_DIR}/sync_component_factory.h
        ${SDK_SRC_DIR}/sync_component_impl.cpp
    )
    
  
endfunction()

set(SRC_PublicHeaders_Component 
    sync_component.h
    sync_component_factory.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component 
    sync_component_impl.h
    sync_component_internal.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    sync_component_impl.cpp
    PARENT_SCOPE
)
