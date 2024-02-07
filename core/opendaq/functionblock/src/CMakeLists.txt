set(BASE_NAME function_block)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_FunctionBlock function_block.h)
rtgen(SRC_FunctionBlockType function_block_type.h)
rtgen(SRC_FunctionBlockWrapper function_block_wrapper.h)
rtgen(SRC_Channel channel.h)

source_group("function_block" FILES ${SDK_HEADERS_DIR}/function_block.h
                                    ${SDK_HEADERS_DIR}/function_block_impl.h
                                    ${SDK_HEADERS_DIR}/function_block_type.h
                                    ${SDK_HEADERS_DIR}/function_block_type_impl.h
                                    ${SDK_HEADERS_DIR}/function_block_type_factory.h
                                    function_block_type_impl.cpp
)

source_group("function_block_wrapper" FILES ${SDK_HEADERS_DIR}/function_block_wrapper.h
                                            ${SDK_HEADERS_DIR}/function_block_wrapper_impl.h
                                            ${SDK_HEADERS_DIR}/function_block_wrapper_factory.h
                                            ${SDK_HEADERS_DIR}/property_wrapper_impl.h
                                            function_block_wrapper_impl.cpp
                                            property_wrapper_impl.cpp
)

source_group("channel" FILES ${SDK_HEADERS_DIR}/channel.h
                             ${SDK_HEADERS_DIR}/channel_impl.h
)

source_group("input_port" FILES ${SDK_HEADERS_DIR}/function_block_input_port_impl.h
                                ${SDK_HEADERS_DIR}/function_block_input_port_factory.h
                                function_block_input_port_impl.cpp
)

set(SRC_Cpp function_block_type_impl.cpp
            function_block_wrapper_impl.cpp
            property_wrapper_impl.cpp
)

set(SRC_PublicHeaders function_block_type_factory.h
                      function_block_errors.h
                      channel_impl.h
                      function_block_impl.h
                      function_block_wrapper_factory.h
)

set(SRC_PrivateHeaders function_block_type_impl.h
                       function_block_wrapper_impl.h
                       property_wrapper_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_FunctionBlock_Cpp}
                    ${SRC_FunctionBlockType_Cpp}
                    ${SRC_Channel_Cpp}
                    ${SRC_FunctionBlockWrapper_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_FunctionBlock_PublicHeaders}
                              ${SRC_FunctionBlockType_PublicHeaders}
                              ${SRC_Channel_PublicHeaders}
                              ${SRC_FunctionBlockWrapper_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_FunctionBlock_PrivateHeaders}
                               ${SRC_FunctionBlockType_PrivateHeaders}
                               ${SRC_Channel_PrivateHeaders}
                               ${SRC_FunctionBlockWrapper_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
)

if (MSVC)
    opendaq_target_compile_options(${BASE_NAME} PRIVATE /bigobj)
endif()

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC
        daq::coreobjects
        daq::signal
        daq::scheduler
        daq::logger
        daq::utility
        daq::component
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>
)
