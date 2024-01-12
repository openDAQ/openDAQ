set(BASE_NAME component)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Component component.h)
rtgen(SRC_SearchFilter search_filter.h)
rtgen(SRC_Folder folder.h)
rtgen(SRC_FolderConfig folder_config.h)
rtgen(SRC_Removable removable.h)
rtgen(SRC_ComponentDeserializeContext component_deserialize_context.h)
rtgen(SRC_DeserializeComponent deserialize_component.h)
rtgen(SRC_RecursiveSearch recursive_search.h)
rtgen(SRC_ComponentPrivate component_private.h)
rtgen(SRC_Tags tags.h)
rtgen(SRC_TagsPrivate tags_private.h)
rtgen(SRC_ComponentStatusContainer component_status_container.h)
rtgen(SRC_ComponentStatusContainerPrivate component_status_container_private.h)

source_group("component" FILES ${SDK_HEADERS_DIR}/component.h
                               ${SDK_HEADERS_DIR}/component_impl.h
                               ${SDK_HEADERS_DIR}/component_ptr.custom.h
                               ${SDK_HEADERS_DIR}/component_factory.h
                               ${SDK_HEADERS_DIR}/component_private.h
                               ${SDK_HEADERS_DIR}/folder.h
                               ${SDK_HEADERS_DIR}/folder_config.h
                               ${SDK_HEADERS_DIR}/folder_impl.h
                               ${SDK_HEADERS_DIR}/folder_factory.h
                               ${SDK_HEADERS_DIR}/component_keys.h
                               ${SDK_HEADERS_DIR}/removable.h
							   ${SDK_HEADERS_DIR}/deserialize_component.h
							   ${SDK_HEADERS_DIR}/component_deserialize_context.h
							   ${SDK_HEADERS_DIR}/component_deserialize_context_impl.h
							   ${SDK_HEADERS_DIR}/component_deserialize_context_factory.h
                               component_impl.cpp
                               folder_impl.cpp
							   component_deserialize_context.cpp
)

source_group("search_filter" FILES ${SDK_HEADERS_DIR}/search_filter.h
                                   ${SDK_HEADERS_DIR}/search_filter_impl.h
                                   ${SDK_HEADERS_DIR}/search_filter_factory.h
                                   ${SDK_HEADERS_DIR}/recursive_search.h
                                   search_filter_impl.cpp
)

source_group("tag" FILES  ${SDK_HEADERS_DIR}/tags_impl.h
                          ${SDK_HEADERS_DIR}/tags.h
                          ${SDK_HEADERS_DIR}/tags_factory.h
                          ${SDK_HEADERS_DIR}/tags_private.h
                          tags_impl.cpp
)

source_group("component_status_container" FILES ${SDK_HEADERS_DIR}/component_status_container.h
                                                ${SDK_HEADERS_DIR}/component_status_container_private.h
                                                ${SDK_HEADERS_DIR}/component_status_container_impl.h
                                                ${SDK_HEADERS_DIR}/component_status_container_factory.h
                                                component_status_container_impl.cpp
)

set(SRC_Cpp 
    component_impl.cpp
    folder_impl.cpp
	component_deserialize_context_impl.cpp
    search_filter_impl.cpp
    tags_impl.cpp
    component_status_container_impl.cpp
)


set(SRC_PublicHeaders 
    component_impl.h
    component_ptr.custom.h
    folder_factory.h
    folder_impl.h
    deserialize_component.h
    component_factory.h
    component_keys.h
	component_deserialize_context_factory.h
    component_deserialize_context_impl.h	
    search_filter_factory.h
    tags_factory.h
    tags_impl.h
    component_status_container.h
    component_status_container_factory.h
    component_status_container_impl.h
)

set(SRC_PrivateHeaders
    search_filter_impl.h
	tags_impl.h      
    component_status_container_private.h
)

prepend_include(${MAIN_TARGET} SRC_PublicHeaders)
prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)

list(APPEND SRC_Cpp ${SRC_Component_Cpp}
                    ${SRC_Folder_Cpp}
                    ${SRC_FolderConfig_Cpp}
                    ${SRC_FolderConfig_Cpp}
					${SRC_ComponentDeserializeContext_Cpp}
                    ${SRC_SearchFilter_Cpp}
					${SRC_Tags_Cpp}
					${SRC_TagsPrivate_Cpp}
                    ${SRC_ComponentStatusContainer_Cpp}
                    ${SRC_ComponentStatusContainerPrivate_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Component_PublicHeaders}
                              ${SRC_Folder_PublicHeaders}
                              ${SRC_FolderConfig_PublicHeaders}
                              ${SRC_IoFolder_PublicHeaders}
                              ${SRC_Removable_PublicHeaders}
                              ${SRC_ComponentDeserializeContext_PublicHeaders}
                              ${SRC_DeserializeComponent_PublicHeaders}
                              ${SRC_SearchFilter_PublicHeaders}
                              ${SRC_RecursiveSearch_PublicHeaders}
                              ${SRC_ComponentPrivate_PublicHeaders}
							  ${SRC_Tags_PublicHeaders}
							  ${SRC_TagsPrivate_PublicHeaders}
                              ${SRC_ComponentStatusContainer_PublicHeaders}
                              ${SRC_ComponentStatusContainerPrivate_PublicHeaders}
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
