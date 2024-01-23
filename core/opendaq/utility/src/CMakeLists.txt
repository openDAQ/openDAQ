set(BASE_NAME utility)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

source_group("sample_type" FILES ${SDK_HEADERS_DIR}/sample_type.h
)

source_group("ids_parser" FILES ${SDK_HEADERS_DIR}/ids_parser.h
                                ids_parser.cpp
)

set(SRC_Cpp ids_parser.cpp
)

set(SRC_PublicHeaders utility_errors.h
                      utility_exceptions.h
                      utility_sync.h
)

set(SRC_PrivateHeaders ids_parser.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp 
)

list(APPEND SRC_PublicHeaders utility.natvis
)

list(APPEND SRC_PrivateHeaders
)

# No bindings are generated in utility. Workaround for install.
get_filename_component(CURR_BINDINGS_DIR
					   "${CMAKE_CURRENT_BINARY_DIR}/../bindings"
					   ABSOLUTE)
opendaq_create_dir(${CURR_BINDINGS_DIR})
	
opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC daq::coreobjects

    # $<BUILD_INTERFACE:Boost::algorithm>
    PRIVATE $<BUILD_INTERFACE:Boost::algorithm>
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>
)
