set(BASE_NAME synchronization)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_SyncComponent sync_component.h)
#rtgen(SRC_SyncComponentConfig sync_component_config.h)

source_group("sync_component" FILES ${SDK_HEADERS_DIR}/sync_component.h
                                    #${SDK_HEADERS_DIR}/sync_component_config.h
                                    ${SDK_HEADERS_DIR}/sync_component_impl.h
                                    ${SDK_HEADERS_DIR}/sync_component_factory.h
                                    sync_component_impl.cpp
)

set(SRC_Cpp sync_component_impl.cpp
)

set(SRC_PublicHeaders
    sync_component_factory.h
)

set(SRC_PrivateHeaders sync_component_impl.h
)

set(SRC_ExtraPublicLibraries)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_SyncComponent_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_SyncComponent_PublicHeaders}
                              #${SRC_SyncComponentConfig_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_SyncComponent_PrivateHeaders}
                               #${SRC_SyncComponentConfig_PrivateHeaders}
)

if (WIN32)
    set(BCRYPT_LIB bcrypt.dll)
endif()

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${BuilderHeaderSource}
)

opendaq_target_link_libraries(${BASE_NAME}
    PRIVATE tsl::ordered_map
            ${BCRYPT_LIB}
            daq::logger
    PUBLIC daq::coreobjects
           daq::scheduler
           daq::utility
           daq::context
           daq::component
    ${SRC_ExtraPublicLibraries}
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>

        $<TARGET_PROPERTY:daq::module_manager,INTERFACE_INCLUDE_DIRECTORIES>
)
