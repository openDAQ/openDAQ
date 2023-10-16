set(BASE_NAME context)
set(LIB_NAME ${SDK_TARGET_NAMESPACE}${BASE_NAME})

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Context context.h)

source_group("context" FILES ${SDK_HEADERS_DIR}/context.h
)

set(SRC_Cpp empty.cpp
)

set(SRC_PublicHeaders
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

add_library(${LIB_NAME} ${OPENDAQ_COMPONENT_TYPE} ${SRC_Cpp}
                                                    ${SRC_PrivateHeaders}
                                                    ${SRC_PublicHeaders}
)

add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME} PUBLIC daq::coretypes
                                         daq::logger
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

                                              $<INSTALL_INTERFACE:include>
                                              $<TARGET_PROPERTY:daq::module_manager,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(${LIB_NAME} PRIVATE BUILDING_SHARED_LIBRARY)
set_target_properties(${LIB_NAME}
    PROPERTIES
        PUBLIC_HEADER "${SRC_PublicHeaders}"
        OPENDAQ_COMPONENT TRUE
)

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /wd4100)
endif()

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME}
                      INCLUDE_FOLDER ${MAIN_TARGET}
                      NO_RC
)

install(TARGETS ${LIB_NAME}
        EXPORT ${SDK_NAME}
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT ${SDK_NAME}_${MAIN_TARGET}_Runtime
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT          ${SDK_NAME}_${MAIN_TARGET}_Runtime
            NAMELINK_COMPONENT ${SDK_NAME}_${MAIN_TARGET}_Development
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ${SDK_NAME}_${MAIN_TARGET}_Development
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MAIN_TARGET}
            COMPONENT ${SDK_NAME}_${MAIN_TARGET}_Development
)

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bindings/"
        DESTINATION bindings
)
