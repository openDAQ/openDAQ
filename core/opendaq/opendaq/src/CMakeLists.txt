set(BASE_NAME ${SDK_TARGET_NAME})
set(LIB_NAME ${BASE_NAME})

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(ConfigHeaderSource ${SDK_HEADERS_DIR}/opendaq_config.h.in)
set(ConfigHeader ${GENERATED_HEADERS_DIR}/opendaq_config.h)

configure_file(${ConfigHeaderSource} ${ConfigHeader})

set(RTGEN_LIBRARY_NAME ${SDK_NAME})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Instance instance.h)

source_group("instance" FILES ${SDK_HEADERS_DIR}/instance.h
                              ${SDK_HEADERS_DIR}/instance_impl.h
                              ${SDK_HEADERS_DIR}/instance_factory.h
                              ${SDK_HEADERS_DIR}/client_private.h
                              ${SDK_HEADERS_DIR}/client_impl.h
                              ${SDK_HEADERS_DIR}/create_device.h
                              instance_impl.cpp
                              client_impl.cpp
                              create_device.cpp
)

set(SRC_Cpp version.cpp
            instance_impl.cpp
            client_impl.cpp
            opendaq_init.cpp
            create_device.cpp
)

set(SRC_PublicHeaders version.h
                      opendaq.h
                      errors.h
                      exceptions.h
                      instance_factory.h
)

set(SRC_PrivateHeaders instance_impl.h
                       client_private.h
                       client_impl.h
                       opendaq_init.h
                       create_device.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Instance_Cpp}
)

list(APPEND SRC_PublicHeaders ${ConfigHeader}
                              ${SRC_Instance_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Instance_PrivateHeaders}
)

add_library(${LIB_NAME} SHARED ${SRC_Cpp}
                               ${SRC_PrivateHeaders}
                               ${SRC_PublicHeaders}
                               ${ConfigHeaderSource}
                               opendaq.natvis
)

add_library(${SDK_TARGET_NAMESPACE}::${BASE_NAME} ALIAS ${LIB_NAME})

rtgen_config(${PROJECT_NAME}
             ${LIB_NAME}
             ${PROJECT_VERSION_MAJOR}
             ${PROJECT_VERSION_MINOR}
             ${PROJECT_VERSION_PATCH}
)

set(PRIVATE_LIBRARIES daq::function_block
                      daq::module_manager
                      daq::scheduler
                      daq::logger
                      daq::device
                      daq::signal
                      daq::reader
                      daq::context
                      daq::utility
                      daq::server
                      daq::component
                      daq::streaming
)

target_link_libraries(${LIB_NAME} PRIVATE ${PRIVATE_LIBRARIES}
                                  PUBLIC daq::coreobjects
)

target_link_libraries(${BASE_NAME}
    PRIVATE
        $<BUILD_INTERFACE:Boost::uuid>
        $<BUILD_INTERFACE:Boost::algorithm>
)

foreach (PRIVATE_LIBRARY ${PRIVATE_LIBRARIES})
    target_link_libraries(${LIB_NAME} PRIVATE $<TARGET_OBJECTS:${PRIVATE_LIBRARY}>)
endforeach()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

                                              $<INSTALL_INTERFACE:include>
)

opendaq_forward_include_headers(${LIB_NAME} PRIVATE_LIBRARIES)

target_compile_definitions(${LIB_NAME} PRIVATE BUILDING_SHARED_LIBRARY
                                       INTERFACE OPENDAQ_LINKS_OPENDAQ
)
set_target_properties(${LIB_NAME}
    PROPERTIES
        PUBLIC_HEADER "${SRC_PublicHeaders}"
        OPENDAQ_COMPONENT TRUE
)

set_source_files_properties(${ConfigHeader} PROPERTIES GENERATED TRUE)
source_group("Generated\\Header Files" FILES ${ConfigHeader})

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /wd4100)
endif()

opendaq_set_output_lib_name(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME} ONLY_RC)

install(TARGETS ${LIB_NAME}
        EXPORT ${SDK_NAME}
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Runtime
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT          ${SDK_NAME}_${BASE_NAME}_Runtime
            NAMELINK_COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${BASE_NAME}
            COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
)

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bindings/"
        DESTINATION bindings
)
