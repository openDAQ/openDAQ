set(BASE_NAME component)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Component component.h)
rtgen(SRC_Folder folder.h)
rtgen(SRC_FolderConfig folder_config.h)
rtgen(SRC_Removable removable.h)

source_group("component" FILES ${SDK_HEADERS_DIR}/component.h
                               ${SDK_HEADERS_DIR}/component_impl.h
                               ${SDK_HEADERS_DIR}/component_ptr.custom.h
                               ${SDK_HEADERS_DIR}/component_factory.h
                               ${SDK_HEADERS_DIR}/folder.h
                               ${SDK_HEADERS_DIR}/folder_config.h
                               ${SDK_HEADERS_DIR}/folder_impl.h
                               ${SDK_HEADERS_DIR}/folder_factory.h
                               ${SDK_HEADERS_DIR}/component_keys.h
                               ${SDK_HEADERS_DIR}/removable.h
                               component_impl.cpp
                               folder_impl.cpp
)


set(SRC_Cpp 
    component_impl.cpp
    folder_impl.cpp
)


set(SRC_PublicHeaders 
    component_impl.h
    component_ptr.custom.h
    folder_factory.h
    folder_impl.h
    component_factory.h
    component_keys.h
)

set(SRC_PrivateHeaders
)

prepend_include(${MAIN_TARGET} SRC_PublicHeaders)
prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)

list(APPEND SRC_Cpp ${SRC_Component_Cpp}
                    ${SRC_Folder_Cpp}
                    ${SRC_FolderConfig_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Component_PublicHeaders}
                              ${SRC_Folder_PublicHeaders}
                              ${SRC_FolderConfig_PublicHeaders}
                              ${SRC_IoFolder_PublicHeaders}
                              ${SRC_Removable_PublicHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PublicHeaders}
    ${SRC_PrivateHeaders}
    ${ConfigHeaderSource}
)

opendaq_target_link_libraries(${BASE_NAME}
    PRIVATE daq::logger
    PUBLIC daq::coreobjects
           daq::scheduler
           daq::utility
           daq::context
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>

        $<TARGET_PROPERTY:daq::context,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:daq::logger,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:daq::scheduler,INTERFACE_INCLUDE_DIRECTORIES>
)
