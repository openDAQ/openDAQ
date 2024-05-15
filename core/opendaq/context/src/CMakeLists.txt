set(BASE_NAME context)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Context context.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_Context_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_Context_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Context_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "context/include/opendaq")
    set(SDK_SRC_DIR "context/src")
    
    source_group("context//context" FILES 
        ${SDK_HEADERS_DIR}/context.h
        ${SDK_HEADERS_DIR}/context_ptr.fwd_declare.h
    )
endfunction()

set(SRC_PublicHeaders_Component 
    context_ptr.fwd_declare.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    PARENT_SCOPE
)

set(SRC_Cpp_Component
    PARENT_SCOPE
)
