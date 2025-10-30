set_cmake_folder_context(TARGET_FOLDER_NAME)

if (MSVC)
    add_compile_options(/wd4100)
endif()

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

if (DAQMODULES_REF_FB_MODULE AND DAQMODULES_REF_DEVICE_MODULE)
    message(STATUS "Reference device & function block module test app")
    add_subdirectory(test_ref_modules)
endif()

if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND
        DAQMODULES_OPENDAQ_SERVER_MODULE AND
        OPENDAQ_ENABLE_WEBSOCKET_STREAMING AND
        OPENDAQ_ENABLE_NATIVE_STREAMING AND
        OPENDAQ_ENABLE_OPCUA AND
        DAQMODULES_REF_FB_MODULE AND
        DAQMODULES_REF_DEVICE_MODULE)
    message(STATUS "${SDK_NAME} device and server module test app")
    add_subdirectory(test_opendaq_device_modules)
endif()

if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND OPENDAQ_ENABLE_WS_SIGGEN_INTEGRATION_TESTS)
    message(STATUS "Websocket streaming signal generator tool integration test app")
    add_subdirectory(test_ws_siggen_integration)
endif()

if (DAQMODULES_LICENSING_MODULE)
	set(LICENSING_MODULE_REQUIREMENTS_MET ON)
	if(LINUX)
		find_package(PkgConfig QUIET)
		if(NOT PKGCONFIG_FOUND)
			message("Cannot build licensing example, PkgConfig not found!")
			set(LICENSING_MODULE_REQUIREMENTS_MET OFF)
		else()
			pkg_check_modules(GPGME QUIET gpgme)
			if(NOT GPGME_FOUND)
				message("Cannot build licensing example, GPGme not found!")
				set(LICENSING_MODULE_REQUIREMENTS_MET OFF)
			endif()
		endif()
	endif()
	if(LICENSING_MODULE_REQUIREMENTS_MET)
		add_subdirectory(test_ref_licensing_implementation)
	endif()
endif()