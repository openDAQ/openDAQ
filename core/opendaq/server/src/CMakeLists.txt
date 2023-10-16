set(BASE_NAME server)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Server server.h)
rtgen(SRC_ServerType server_type.h)

source_group("server" FILES ${SDK_HEADERS_DIR}/server.h
                            ${SDK_HEADERS_DIR}/server_impl.h
                            ${SDK_HEADERS_DIR}/server_type.h
                            ${SDK_HEADERS_DIR}/server_type_impl.h
                            ${SDK_HEADERS_DIR}/server_type_factory.h
                            server_type_impl.cpp
                            server_impl.cpp
)

set(SRC_Cpp server_type_impl.cpp
)

set(SRC_PublicHeaders server_type_factory.h
)

set(SRC_PrivateHeaders server_type_impl.h
                       server_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Server_Cpp}
                    ${SRC_ServerType_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Server_PublicHeaders}
                              ${SRC_ServerType_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Server_PrivateHeaders}
                               ${SRC_ServerType_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC
        daq::coreobjects
        daq::logger
        daq::utility
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>

        $<TARGET_PROPERTY:daq::device,INTERFACE_INCLUDE_DIRECTORIES>
)

opendaq_add_dependencies(${BASE_NAME} daq::device)
