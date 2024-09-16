set(BASE_NAME component)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Component component.h)
    rtgen(SRC_SearchFilter search_filter.h)
    rtgen(SRC_Folder folder.h)
    rtgen(SRC_FolderConfig folder_config.h)
    rtgen(SRC_Removable removable.h)
    rtgen(SRC_ComponentDeserializeContext component_deserialize_context.h INTERNAL)
    rtgen(SRC_DeserializeComponent deserialize_component.h)
    rtgen(SRC_RecursiveSearch recursive_search.h)
    rtgen(SRC_ComponentPrivate component_private.h)
    rtgen(SRC_Tags tags.h)
    rtgen(SRC_TagsPrivate tags_private.h)
    rtgen(SRC_ComponentStatusContainer component_status_container.h)
    rtgen(SRC_ComponentStatusContainerPrivate component_status_container_private.h)
    rtgen(SRC_ComponentHolder component_holder.h)
    rtgen(SRC_ComponentTypeBuilder component_type_builder.h)
    rtgen(SRC_ComponentUpdateContext component_update_context.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_Component_PublicHeaders}
        ${SRC_Folder_PublicHeaders}
        ${SRC_FolderConfig_PublicHeaders}
        ${SRC_IoFolder_PublicHeaders}
        ${SRC_Removable_PublicHeaders}
        ${SRC_ComponentDeserializeContext_PublicHeaders}
        ${SRC_DeserializeComponent_PublicHeaders}
        ${SRC_SearchFilter_PublicHeaders}
        ${SRC_RecursiveSearch_PublicHeaders}
        ${SRC_ComponentPrivate_PublicHeaders}
        ${SRC_Tags_PublicHeaders}
        ${SRC_TagsPrivate_PublicHeaders}
        ${SRC_ComponentStatusContainer_PublicHeaders}
        ${SRC_ComponentStatusContainerPrivate_PublicHeaders}
        ${SRC_ComponentHolder_PublicHeaders}
        ${SRC_ComponentTypeBuilder_PublicHeaders}
        ${SRC_ComponentUpdateContext_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_ComponentHolder_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Component_Cpp}
        ${SRC_Folder_Cpp}
        ${SRC_FolderConfig_Cpp}
        ${SRC_FolderConfig_Cpp}
        ${SRC_ComponentDeserializeContext_Cpp}
        ${SRC_SearchFilter_Cpp}
        ${SRC_Tags_Cpp}
        ${SRC_TagsPrivate_Cpp}
        ${SRC_ComponentStatusContainer_Cpp}
        ${SRC_ComponentStatusContainerPrivate_Cpp}
        ${SRC_ComponentHolder_Cpp}
        ${SRC_ComponentUpdateContext_Cpp}
        PARENT_SCOPE
    )

endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "component/include/opendaq")
    set(SDK_SRC_DIR "component/src")
    
    source_group("component//component" FILES 
        ${SDK_HEADERS_DIR}/component.h
        ${SDK_HEADERS_DIR}/component_impl.h
        ${SDK_HEADERS_DIR}/component_ptr.custom.h
        ${SDK_HEADERS_DIR}/component_factory.h
        ${SDK_HEADERS_DIR}/component_private.h
        ${SDK_HEADERS_DIR}/folder.h
        ${SDK_HEADERS_DIR}/folder_config.h
        ${SDK_HEADERS_DIR}/folder_impl.h
        ${SDK_HEADERS_DIR}/folder_factory.h
        ${SDK_HEADERS_DIR}/component_keys.h
        ${SDK_HEADERS_DIR}/removable.h
        ${SDK_HEADERS_DIR}/deserialize_component.h
        ${SDK_HEADERS_DIR}/component_deserialize_context.h
        ${SDK_HEADERS_DIR}/component_deserialize_context_impl.h
        ${SDK_HEADERS_DIR}/component_deserialize_context_factory.h
        ${SDK_HEADERS_DIR}/component_holder_factory.h
        ${SDK_SRC_DIR}/component_impl.cpp
        ${SDK_SRC_DIR}/folder_impl.cpp
        ${SDK_SRC_DIR}/component_deserialize_context_impl.cpp
        ${SDK_HEADERS_DIR}/component_update_context.h
        ${SDK_HEADERS_DIR}/component_update_context_impl.h
    )
    
    source_group("component//search_filter" FILES 
        ${SDK_HEADERS_DIR}/search_filter.h
        ${SDK_HEADERS_DIR}/search_filter_impl.h
        ${SDK_HEADERS_DIR}/search_filter_factory.h
        ${SDK_HEADERS_DIR}/recursive_search.h
        ${SDK_SRC_DIR}/search_filter_impl.cpp
    )
    
    source_group("component//tag" FILES  
        ${SDK_HEADERS_DIR}/tags_impl.h
        ${SDK_HEADERS_DIR}/tags.h
        ${SDK_HEADERS_DIR}/tags_factory.h
        ${SDK_HEADERS_DIR}/tags_private.h
        ${SDK_SRC_DIR}/tags_impl.cpp
    )
    
    source_group("component//component_status_container" FILES 
        ${SDK_HEADERS_DIR}/component_status_container.h
        ${SDK_HEADERS_DIR}/component_status_container_private.h
        ${SDK_HEADERS_DIR}/component_status_container_impl.h
        ${SDK_HEADERS_DIR}/component_status_container_factory.h
        ${SDK_SRC_DIR}/component_status_container_impl.cpp
    )
    
    source_group("component//component_holder" FILES 
        ${SDK_HEADERS_DIR}/component_holder.h
        ${SDK_HEADERS_DIR}/component_holder_impl.h
        ${SDK_SRC_DIR}/component_holder_impl.cpp
    )
    
    source_group("component_type" FILES ${SDK_HEADERS_DIR}/component_type_builder.h
                                    ${SDK_HEADERS_DIR}/component_type_builder_factory.h
    )
endfunction()

set(SRC_PublicHeaders_Component 
    component_impl.h
    component_ptr.custom.h
    folder_factory.h
    folder_impl.h
    deserialize_component.h
    component_factory.h
    component_keys.h
    component_deserialize_context_factory.h
    component_deserialize_context_impl.h    
    search_filter_factory.h
    tags_factory.h
    tags_impl.h
    component_status_container.h
    component_status_container_factory.h
    component_status_container_impl.h
    component_holder_factory.h
    component_errors.h
    component_exceptions.h
    component_type_builder_factory.h
    component_update_context.h
    component_update_context_impl.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    search_filter_impl.h
    tags_impl.h      
    component_status_container_private.h
    component_holder_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    component_impl.cpp
    folder_impl.cpp
    component_deserialize_context_impl.cpp
    search_filter_impl.cpp
    tags_impl.cpp
    component_status_container_impl.cpp
    component_holder_impl.cpp
    PARENT_SCOPE
)
