set(BASE_NAME server)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Server server.h)
    rtgen(SRC_ServerType server_type.h)
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_Server_PublicHeaders}
        ${SRC_ServerType_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_Server_PrivateHeaders}
        ${SRC_ServerType_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Server_Cpp}
        ${SRC_ServerType_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "server/include/opendaq")
    set(SDK_SRC_DIR "server/src")
    
    source_group("server" FILES 
        ${SDK_HEADERS_DIR}/server.h
        ${SDK_HEADERS_DIR}/server_impl.h
        ${SDK_HEADERS_DIR}/server_type.h
        ${SDK_HEADERS_DIR}/server_type_impl.h
        ${SDK_HEADERS_DIR}/server_type_factory.h
        ${SDK_SRC_DIR}/server_type_impl.cpp
        ${SDK_SRC_DIR}/server_impl.cpp
    )
endfunction()

set(SRC_PublicHeaders_Component
    server_type_factory.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component 
    server_type_impl.h
    server_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    server_type_impl.cpp
    server.natvis
    PARENT_SCOPE
)
