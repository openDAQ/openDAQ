set(BASE_NAME utility)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Tags tags.h)
rtgen(SRC_TagsConfig tags_config.h)

source_group("tag" FILES  ${SDK_HEADERS_DIR}/tags_impl.h
                          ${SDK_HEADERS_DIR}/tags.h
                          ${SDK_HEADERS_DIR}/tags_config.h
                          ${SDK_HEADERS_DIR}/tags_factory.h
                          tags_impl.cpp
)

source_group("sample_type" FILES ${SDK_HEADERS_DIR}/sample_type.h
)

source_group("ids_parser" FILES ${SDK_HEADERS_DIR}/ids_parser.h
                                ids_parser.cpp
)

set(SRC_Cpp tags_impl.cpp
            ids_parser.cpp
)

set(SRC_PublicHeaders utility_errors.h
                      utility_exceptions.h
                      utility_sync.h
					  tags_factory.h
)

set(SRC_PrivateHeaders tags_impl.h
                       ids_parser.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Tags_Cpp}
                    ${SRC_TagsConfig_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Tags_PublicHeaders}
                              ${SRC_TagsConfig_PublicHeaders}
                              utility.natvis
)

list(APPEND SRC_PrivateHeaders ${SRC_Tags_PrivateHeaders}
                               ${SRC_TagsConfig_PrivateHeaders}
)

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
