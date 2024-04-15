set(BASE_NAME module_manager)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_Module module.h)
rtgen(SRC_ModuleManager module_manager.h)
rtgen(SRC_ModuleManagerUtils module_manager_utils.h)
rtgen(SRC_ContextInternal context_internal.h)

source_group("module" FILES ${SDK_HEADERS_DIR}/module.h
                            ${SDK_HEADERS_DIR}/module_impl.h
                            ${SDK_HEADERS_DIR}/module_exports.h
                            ${SDK_HEADERS_DIR}/module_factory.h
                            ${SDK_HEADERS_DIR}/module_check_dependencies.h
                            module_impl.cpp
)

source_group("manager" FILES ${SDK_HEADERS_DIR}/module_manager.h
                             ${SDK_HEADERS_DIR}/module_manager_impl.h
                             ${SDK_HEADERS_DIR}/module_manager_init.h
                             ${SDK_HEADERS_DIR}/module_manager_utils.h
                             ${SDK_HEADERS_DIR}/module_manager_factory.h
                             module_manager_impl.cpp
)

source_group("context" FILES ${SDK_HEADERS_DIR}/context_impl.h
                             ${SDK_HEADERS_DIR}/context_factory.h
                             ${SDK_HEADERS_DIR}/context_internal.h
                             context_impl.cpp
)

source_group("errors" FILES ${SDK_HEADERS_DIR}/module_manager_errors.h
                            ${SDK_HEADERS_DIR}/module_manager_exceptions.h
)

set(SRC_Cpp module_manager_impl.cpp
            module_manager_init.cpp

            context_impl.cpp
            orphaned_modules.cpp
)

set(SRC_PublicHeaders module_manager_errors.h
                      module_manager_exceptions.h
                      module_manager_factory.h

                      module_check_dependencies.h
                      module_factory.h
                      module_exports.h
                      module_impl.h

                      context_factory.h
)

set(SRC_PrivateHeaders module_library.h
                       orphaned_modules.h
                       module_manager_impl.h
                       module_manager_init.h
                       boost_dll.h
                       context_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Module_Cpp}
                    ${SRC_ModuleManager_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Module_PublicHeaders}
                              ${SRC_ModuleManager_PublicHeaders}
                              ${SRC_ModuleManagerUtils_PublicHeaders}
                              ${SRC_ContextInternal_PublicHeaders}
                              module_manager.natvis
)

list(APPEND SRC_PrivateHeaders ${SRC_Module_PrivateHeaders}
                               ${SRC_ModuleManager_PrivateHeaders}
                               ${SRC_ContextInternal_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
)

if("${Boost_LIBRARIES}")
    set(BOOST_LIBS ${Boost_LIBRARIES})
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(STDCFS_LIBS "stdc++fs")
endif()

opendaq_target_link_libraries(${BASE_NAME}
    PRIVATE
        ${BOOST_LIBS}
        ${CMAKE_DL_LIBS}
        ${STDCFS_LIBS}
        std::filesystem
        daq::context
        daq::logger
        daq::utility
        daq::component
        daq::discovery_ping
    PUBLIC
        daq::coretypes
)

# we shouldn't link everything, only the necessary bits $<BUILD_INTERFACE:Boost::dll>
opendaq_target_link_libraries(${BASE_NAME} PRIVATE $<BUILD_INTERFACE:Boost::dll>)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>

        $<TARGET_PROPERTY:daq::device,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:daq::server,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:daq::function_block,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:daq::logger,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:daq::scheduler,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:daq::streaming,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE
        ${Boost_INCLUDE_DIRS}
)

opendaq_target_compile_definitions(${BASE_NAME}
    PRIVATE
        BOOST_DLL_USE_STD_FS
)

# Make sure these are built before but do not link them directly
set(DEPENDENT_TARGETS daq::device
                      daq::server
                      daq::function_block
                      daq::scheduler
                      daq::streaming
)

opendaq_add_dependencies(${BASE_NAME} ${DEPENDENT_TARGETS})
