set(LIC_EXAMPLE licensing_example)

if (LINUX)
	set(MODULE_AUTHENTICATOR_HEADER module_authenticator_impl_linux.h)
	set(MODULE_AUTHENTICATOR_SOURCE module_authenticator_impl_linux.cpp)
endif (LINUX)
if (WIN32)
	set(MODULE_AUTHENTICATOR_HEADER module_authenticator_impl_win.h)
	set(MODULE_AUTHENTICATOR_SOURCE module_authenticator_impl_win.cpp)
endif (WIN32)

set(SRC_Headers ${MODULE_AUTHENTICATOR_HEADER}
)

set(SRC_Cpp main.cpp
            ${MODULE_AUTHENTICATOR_SOURCE}
)

opendaq_prepend_include(${TARGET_FOLDER_NAME} SRC_Headers)
add_executable(${LIC_EXAMPLE} ${SRC_Headers}
                              ${SRC_Cpp}
)

if (MSVC)
    target_compile_options(${LIC_EXAMPLE} 
        PRIVATE 
            /bigobj 
            /wd4100     #Avoid 'unreferenced formal parameter' compiler warning  (Otherwise will not compile in VS in Release build...)
            /ZI         #Enable Hot Reload per default (see: ) https://learn.microsoft.com/en-us/visualstudio/debugger/edit-and-continue-visual-cpp?view=vs-2022#BKMK_Enable_or_disable_automatic_invocation_of_Edit_and_Continue
    )
endif()

if (WIN32)
    target_link_libraries(${LIC_EXAMPLE}
        PRIVATE 
            wintrust        # Required for License Certificate verification
            crypt32         # Required for License Certificate verification
    )
endif()

if (LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GPGME REQUIRED gpgme)

    target_include_directories(${LIC_EXAMPLE} PRIVATE ${GPGME_INCLUDE_DIRS})
    target_link_libraries(${LIC_EXAMPLE} PRIVATE ${GPGME_LIBRARIES})
endif()

target_include_directories(${LIC_EXAMPLE} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
                                               $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/>
)

target_link_libraries(${LIC_EXAMPLE} PRIVATE daq::opendaq
)

add_dependencies(${LIC_EXAMPLE} daq::licensing_module
)

