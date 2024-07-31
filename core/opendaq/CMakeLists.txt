set(MAIN_TARGET opendaq)
set_cmake_folder_context(TARGET_FOLDER_NAME)

include(DaqComponents)
set(RTGEN_LIBRARY_NAME ${SDK_NAME})
project(${SDK_NAME} VERSION ${OPENDAQ_CORE_VERSION} LANGUAGES C CXX)

set(DAQ_COMPONENTS 
    logger
    scheduler
    context
    signal
    functionblock
    device
    server
    modulemanager
    reader
    utility
    component
    streaming
    synchronization
    opendaq
)

foreach(DAQ_COMPONENT ${DAQ_COMPONENTS})
    set(SRC_PublicHeaders_Component "")
    set(SRC_PrivateHeaders_Component "")
    set(SRC_Cpp_Component "")
    set(SRC_PublicHeaders_Component_Generated "")
    set(SRC_PrivateHeaders_Component_Generated "")
    set(SRC_Cpp_Component_Generated "")
    set(SRC_PublicLinkLibraries_Component "")
    set(SRC_PrivateLinkLibraries_Component "")
    set(SRC_PublicCompileDefinitions_Component "")
    set(SRC_PrivateCompileDefinitions_Component "")
    set(SRC_PublicCompileOptions_Component "")
    set(SRC_PrivateCompileOptions_Component "")
    set(SRC_PublicIncludeDirectories_Component "")
    set(SRC_PrivateIncludeDirectories_Component "")

    add_subdirectory(${DAQ_COMPONENT}/src)
    
    list(TRANSFORM SRC_PublicHeaders_Component PREPEND "${DAQ_COMPONENT}/include/${MAIN_TARGET}/")
    list(TRANSFORM SRC_PrivateHeaders_Component PREPEND "${DAQ_COMPONENT}/include/${MAIN_TARGET}/")
    list(TRANSFORM SRC_Cpp_Component PREPEND "${DAQ_COMPONENT}/src/")
    
    list(APPEND SRC_PublicHeaders ${SRC_PublicHeaders_Component})
    list(APPEND SRC_PrivateHeaders ${SRC_PrivateHeaders_Component})
    list(APPEND SRC_Cpp ${SRC_Cpp_Component})
    
    list(APPEND SRC_PublicLinkLibraries ${SRC_PublicLinkLibraries_Component})
    list(APPEND SRC_PrivateLinkLibraries ${SRC_PrivateLinkLibraries_Component})
    
    list(APPEND SRC_PublicCompileDefinitions ${SRC_PublicCompileDefinitions_Component})
    list(APPEND SRC_PrivateCompileDefinitions ${SRC_PrivateCompileDefinitions_Component})
    
    list(APPEND SRC_PublicCompileOptions ${SRC_PublicCompileOptions_Component})
    list(APPEND SRC_PrivateCompileOptions ${SRC_PrivateCompileOptions_Component})
    
    list(APPEND SRC_PublicIncludeDirectories
        ${SRC_PublicIncludeDirectories_Component}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${DAQ_COMPONENT}/include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/${DAQ_COMPONENT}/include/>
    )
    
    list(APPEND SRC_PrivateIncludeDirectories ${SRC_PrivateIncludeDirectories_Component})
    
    set(RTGEN_HEADERS_DIR ${DAQ_COMPONENT}/include/${MAIN_TARGET})
    set(RTGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DAQ_COMPONENT}/include/${MAIN_TARGET})
    set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DAQ_COMPONENT}/src)
    set(RTGEN_CURR_BINDINGS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DAQ_COMPONENT}/bindings)
	set(RTGEN_RELATIVE_PARENT_DIR core/${MAIN_TARGET}/${DAQ_COMPONENT})
    
    set(RTGEN_COMPONENT_COMMAND_NAME "rtgen_component_${DAQ_COMPONENT}")
    cmake_language(CALL ${RTGEN_COMPONENT_COMMAND_NAME})
	
    set(CREATE_SOURCE_GROUPS_COMPONENT_COMMAND_NAME "create_component_source_groups_${DAQ_COMPONENT}")
    cmake_language(CALL ${CREATE_SOURCE_GROUPS_COMPONENT_COMMAND_NAME})

    list(APPEND SRC_PublicHeaders ${SRC_PublicHeaders_Component_Generated})
    list(APPEND SRC_PrivateHeaders ${SRC_PrivateHeaders_Component_Generated})
    list(APPEND SRC_Cpp ${SRC_Cpp_Component_Generated})
endforeach()

opendaq_add_library(${MAIN_TARGET} SHARED
        ${SRC_Cpp}
        ${SRC_PrivateHeaders}
        ${SRC_PublicHeaders}
)

foreach(DAQ_COMPONENT ${DAQ_COMPONENTS}) 
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${DAQ_COMPONENT}/bindings/"
            DESTINATION bindings
    )
endforeach()

message(${PROJECT_NAME})

set(RTGEN_CURR_BINDINGS_DIR ${CMAKE_CURRENT_BINARY_DIR}/bindings)
set(RTGEN_RELATIVE_PARENT_DIR core/${MAIN_TARGET})
rtgen_config(${PROJECT_NAME}
             ${MAIN_TARGET}
             ${PROJECT_VERSION_MAJOR}
             ${PROJECT_VERSION_MINOR}
             ${PROJECT_VERSION_PATCH}
)

opendaq_target_link_libraries(${MAIN_TARGET}
    PUBLIC
        daq::coreobjects
        ${SRC_PublicLinkLibraries}
    PRIVATE
        ${SRC_PrivateLinkLibraries}
)

opendaq_target_compile_definitions(${MAIN_TARGET} 
    PUBLIC 
        ${SRC_PublicCompileDefinitions}
    PRIVATE
        BUILDING_SHARED_LIBRARY
        ${SRC_PrivateCompileDefinitions}
    INTERFACE
        OPENDAQ_LINKS_OPENDAQ
)
 
opendaq_target_include_directories(${MAIN_TARGET}
    PUBLIC
        ${SRC_PublicIncludeDirectories}
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${SRC_PrivateIncludeDirectories}
)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(STDCFS_LIBS "stdc++fs")
endif()

if (MSVC)
    set(MSVC_PRIVATE_COMPILE_OPTIONS 
        /wd4324 
        /bigobj
    )
endif()

opendaq_target_compile_options(${MAIN_TARGET} 
    PUBLIC 
        ${SRC_PublicCompileOptions}
    PRIVATE
        ${SRC_PrivateCompileOptions}
        ${MSVC_PRIVATE_COMPILE_OPTIONS}
)

if(OPENDAQ_ENABLE_TESTS)
    add_subdirectory(opendaq/mocks)
    foreach(DAQ_COMPONENT ${DAQ_COMPONENTS}) 
        add_subdirectory(${DAQ_COMPONENT}/tests)
    endforeach()
endif()