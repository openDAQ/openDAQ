set(BASE_NAME context)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Context context.h)

source_group("context" FILES ${SDK_HEADERS_DIR}/context.h
                             ${SDK_HEADERS_DIR}/context_ptr.fwd_declare.h
)

set(SRC_Cpp empty.cpp
)

set(SRC_PublicHeaders context_ptr.fwd_declare.h
)

set(SRC_PrivateHeaders
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Context_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Context_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Context_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC daq::coretypes
           daq::logger
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

           $<INSTALL_INTERFACE:include>
           $<TARGET_PROPERTY:daq::module_manager,INTERFACE_INCLUDE_DIRECTORIES>
)
