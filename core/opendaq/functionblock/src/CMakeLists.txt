set(BASE_NAME functionblock)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_FunctionBlock function_block.h)
    rtgen(SRC_FunctionBlockType function_block_type.h)
    rtgen(SRC_FunctionBlockWrapper function_block_wrapper.h)
    rtgen(SRC_Channel channel.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_FunctionBlock_PublicHeaders}
        ${SRC_FunctionBlockType_PublicHeaders}
        ${SRC_Channel_PublicHeaders}
        ${SRC_FunctionBlockWrapper_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_FunctionBlock_PrivateHeaders}
        ${SRC_FunctionBlockType_PrivateHeaders}
        ${SRC_Channel_PrivateHeaders}
        ${SRC_FunctionBlockWrapper_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_FunctionBlock_Cpp}
        ${SRC_FunctionBlockType_Cpp}
        ${SRC_Channel_Cpp}
        ${SRC_FunctionBlockWrapper_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "functionblock/include/opendaq")
    set(SDK_SRC_DIR "functionblock/src")
    
    source_group("function_block//function_block" FILES 
        ${SDK_HEADERS_DIR}/function_block.h
        ${SDK_HEADERS_DIR}/function_block_impl.h
        ${SDK_HEADERS_DIR}/function_block_type.h
        ${SDK_HEADERS_DIR}/function_block_type_impl.h
        ${SDK_HEADERS_DIR}/function_block_type_factory.h
        ${SDK_SRC_DIR}/function_block_type_impl.cpp
    )
    
    source_group("function_block//function_block_wrapper" FILES 
        ${SDK_HEADERS_DIR}/function_block_wrapper.h
        ${SDK_HEADERS_DIR}/function_block_wrapper_impl.h
        ${SDK_HEADERS_DIR}/function_block_wrapper_factory.h
        ${SDK_HEADERS_DIR}/property_wrapper_impl.h
        ${SDK_SRC_DIR}/function_block_wrapper_impl.cpp
        ${SDK_SRC_DIR}/property_wrapper_impl.cpp
    )
    
    source_group("function_block//channel" FILES 
        ${SDK_HEADERS_DIR}/channel.h
        ${SDK_HEADERS_DIR}/channel_impl.h
    )
    
    source_group("function_block//input_port" FILES 
        ${SDK_HEADERS_DIR}/function_block_input_port_impl.h
        ${SDK_HEADERS_DIR}/function_block_input_port_factory.h
        ${SDK_SRC_DIR}/function_block_input_port_impl.cpp
    )
endfunction()

set(SRC_PublicHeaders_Component 
    function_block_type_factory.h
    function_block_errors.h
    channel_impl.h
    function_block_impl.h
    function_block_wrapper_factory.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    function_block_type_impl.h
    function_block_wrapper_impl.h
    property_wrapper_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    function_block_type_impl.cpp
    function_block_wrapper_impl.cpp
    property_wrapper_impl.cpp
    PARENT_SCOPE
)
