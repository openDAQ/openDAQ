set(BASE_NAME utility)

function(rtgen_component_${BASE_NAME})

endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "utility/include/opendaq")
    set(SDK_SRC_DIR "utility/src")
    
    source_group("utility//sample_type" FILES 
        ${SDK_HEADERS_DIR}/sample_type.h
    )
    
    source_group("utility//ids_parser" FILES 
        ${SDK_HEADERS_DIR}/ids_parser.h
        ${SDK_SRC_DIR}/ids_parser.cpp
    )
    
    source_group("utility" FILES 
        ${SDK_HEADERS_DIR}/utility_errors.h
        ${SDK_HEADERS_DIR}/utility_exceptions.h
        ${SDK_SRC_DIR}/utility.natvis
    )
endfunction()

set(SRC_Cpp_Component 
    ids_parser.cpp
    utility.natvis
    PARENT_SCOPE
)

set(SRC_PublicHeaders_Component 
    utility_errors.h
    utility_exceptions.h
    ids_parser.h
    PARENT_SCOPE
)

set(SRC_PrivateLinkLibraries_Component
    $<BUILD_INTERFACE:Boost::algorithm>
    PARENT_SCOPE
)

# No bindings are generated in utility. Workaround for install.
get_filename_component(CURR_BINDINGS_DIR
                       "${CMAKE_CURRENT_BINARY_DIR}/../bindings"
                       ABSOLUTE)
opendaq_create_dir(${CURR_BINDINGS_DIR})