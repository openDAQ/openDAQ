set_cmake_folder_context(TARGET_FOLDER_NAME documentation)

option(OPENDAQ_DOCUMENTATION_TESTS "Enable ${SDK_NAME} documentation tests" OFF)

option(OPENDAQ_BUILD_DOCUMENTATION "Build Doxygen documentation" OFF)

if (OPENDAQ_BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxygen/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        message(STATUS "Generating Doxygen project")

        add_custom_target(doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
		
		add_dependencies(doc_doxygen daq::opendaq)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen needs to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (OPENDAQ_BUILD_DOCUMENTATION)

if (OPENDAQ_DOCUMENTATION_TESTS AND
    NOT (DAQMODULES_OPENDAQ_CLIENT_MODULE AND
    DAQMODULES_OPENDAQ_SERVER_MODULE AND
	DAQMODULES_REF_FB_MODULE AND
	DAQMODULES_REF_DEVICE_MODULE AND
	OPENDAQ_ENABLE_TESTS))
	message(WARNING "DAQMODULES_OPENDAQ_CLIENT_MODULE, DAQMODULES_OPENDAQ_SERVER_MODULE, DAQMODULES_REF_FB_MODULE, DAQMODULES_REF_DEVICE_MODULE and OPENDAQ_ENABLE_TESTS must be ON to enable documentation tests")
endif()

if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND
    DAQMODULES_OPENDAQ_SERVER_MODULE AND
	DAQMODULES_REF_FB_MODULE AND
	DAQMODULES_REF_DEVICE_MODULE AND
	OPENDAQ_ENABLE_TESTS AND
	OPENDAQ_DOCUMENTATION_TESTS)
    message(STATUS "${SDK_NAME} documentation testing")
    add_subdirectory(tests)
endif()
