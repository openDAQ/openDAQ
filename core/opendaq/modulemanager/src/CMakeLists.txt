set(BASE_NAME modulemanager)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_Module module.h)
    rtgen(SRC_ModuleManager module_manager.h)
    rtgen(SRC_ModuleManagerUtils module_manager_utils.h)
    rtgen(SRC_ContextInternal context_internal.h)
    rtgen(SRC_DiscoveryServer discovery_server.h)
    rtgen(SRC_ModuleInfo module_info.h)

    set(SRC_PublicHeaders_Component_Generated
        ${SRC_Module_PublicHeaders}
        ${SRC_ModuleManager_PublicHeaders}
        ${SRC_ModuleManagerUtils_PublicHeaders}
        ${SRC_ContextInternal_PublicHeaders}
        ${SRC_DiscoveryServer_PublicHeaders}
        ${SRC_ModuleInfo_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_Module_PrivateHeaders}
        ${SRC_ModuleManager_PrivateHeaders}
        ${SRC_ContextInternal_PrivateHeaders}
        ${SRC_DiscoveryServer_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Module_Cpp}
        ${SRC_ModuleManager_Cpp}
        ${SRC_DiscoveryServer_Cpp}
        ${SRC_ModuleInfo_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "modulemanager/include/opendaq")
    set(SDK_SRC_DIR "modulemanager/src")
    
    source_group("module_manager//module" FILES 
        ${SDK_HEADERS_DIR}/module.h
        ${SDK_HEADERS_DIR}/module_impl.h
        ${SDK_HEADERS_DIR}/module_exports.h
        ${SDK_HEADERS_DIR}/module_factory.h
        ${SDK_HEADERS_DIR}/module_check_dependencies.h
        ${SDK_SRC_DIR}/module_impl.cpp
    )
    
    source_group("module_manager//manager" FILES 
        ${SDK_HEADERS_DIR}/module_manager.h
        ${SDK_HEADERS_DIR}/module_manager_impl.h
        ${SDK_HEADERS_DIR}/module_manager_init.h
        ${SDK_HEADERS_DIR}/module_manager_utils.h
        ${SDK_HEADERS_DIR}/module_manager_factory.h
        ${SDK_SRC_DIR}/module_manager_impl.cpp
    )
    
    source_group("module_manager//errors" FILES 
        ${SDK_HEADERS_DIR}/module_manager_errors.h
        ${SDK_HEADERS_DIR}/module_manager_exceptions.h
    )
    
    source_group("module_manager//context" FILES 
        ${SDK_HEADERS_DIR}/context_impl.h
        ${SDK_HEADERS_DIR}/context_factory.h
        ${SDK_HEADERS_DIR}/context_internal.h
        ${SDK_SRC_DIR}/context_impl.cpp
    )
	
    source_group("module_manager//info" FILES 
        ${SDK_HEADERS_DIR}/module_info.h
        ${SDK_HEADERS_DIR}/module_info_impl.h
        ${SDK_HEADERS_DIR}/module_info_factory.h
        ${SDK_SRC_DIR}/module_info_impl.cpp
    )
    
    source_group("module_manager//ping" FILES 
        ${SDK_HEADERS_DIR}/format.h
        ${SDK_HEADERS_DIR}/ipv4_header.h
        ${SDK_HEADERS_DIR}/icmp_header.h
        ${SDK_HEADERS_DIR}/icmp_ping.h
        ${SDK_SRC_DIR}/ipv4_header.cpp
        ${SDK_SRC_DIR}/icmp_header.cpp
        ${SDK_SRC_DIR}/icmp_ping.cpp
    )
	
	source_group("discovery_server" FILES  
        ${SDK_HEADERS_DIR}/discovery_server.h
        ${SDK_HEADERS_DIR}/discovery_server_factory.h
        ${SDK_HEADERS_DIR}/mdns_discovery_server_impl.h
        ${SDK_SRC_DIR}/mdns_discovery_server_impl.cpp
    )
									   
endfunction()

set(SRC_PublicHeaders_Component 
    module_manager_errors.h
    module_manager_exceptions.h
    module_manager_factory.h
    module_check_dependencies.h
    module_factory.h
    module_exports.h
    module_impl.h
    context_factory.h
    format.h
    ipv4_header.h
    icmp_header.h
    icmp_ping.h
    discovery_server_factory.h
    module_info.h
    module_info_factory.h
    module_info_impl.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    module_library.h
    orphaned_modules.h
    module_manager_impl.h
    module_manager_init.h
    boost_dll.h
    context_impl.h
    mdns_discovery_server_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    module_manager_impl.cpp
    module_manager_init.cpp
    context_impl.cpp
    orphaned_modules.cpp
    ipv4_header.cpp
    icmp_header.cpp
    icmp_ping.cpp
    mdns_discovery_server_impl.cpp
    module_info_impl.cpp
    module_manager.natvis
    PARENT_SCOPE
)

if("${Boost_LIBRARIES}")
    set(BOOST_LIBS ${Boost_LIBRARIES})
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(STDCFS_LIBS "stdc++fs")
endif()

if (WIN32)
    set(PING_COMPILE_DEFS
        _WIN32_WINNT=${MIN_WINDOWS_VERSION}
        WINVER=${MIN_WINDOWS_VERSION}
        WIN32_LEAN_AND_MEAN
    )    

    set(PING_LIBS
        Ws2_32
        IPHLPAPI
    )
endif()

set(SRC_PrivateLinkLibraries_Component
    ${BOOST_LIBS}
    ${CMAKE_DL_LIBS}
    ${STDCFS_LIBS}
	${PING_LIBS}
    $<BUILD_INTERFACE:Boost::dll>
    $<BUILD_INTERFACE:Boost::asio>
    $<BUILD_INTERFACE:Threads::Threads>
    $<BUILD_INTERFACE:fmt::fmt>
    $<BUILD_INTERFACE:daq::discovery_server>
    PARENT_SCOPE
)

set(SRC_PrivateIncludeDirectories_Component
    ${Boost_INCLUDE_DIRS}
    PARENT_SCOPE
)

set(SRC_PrivateCompileDefinitions_Component
    ${Boost_INCLUDE_DIRS}
    ${PING_COMPILE_DEFS}
	BOOST_DLL_USE_STD_FS
	BOOST_ALL_NO_LIB
    PARENT_SCOPE
)
