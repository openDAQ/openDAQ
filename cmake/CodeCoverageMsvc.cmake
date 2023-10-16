# Check prereqs
find_program(CPPCOV_PATH OpenCppCoverage)

if(CPPCOV_PATH-NOTFOUND)
    message(FATAL_ERROR "OpenCppCoverage not found! Aborting...")
else()
    # Param _targetname     The name of the new coverage project
    # Param _testrunner     The name of the target which runs the tests.
    #                       MUST return ZERO always, even on errors.
    #                       If not, no coverage report will be created!
    # Param _outputname     Not used
    #
    # Emits coverage information into the projects subfolder "coverage"
    #
    function(SETUP_TARGET_FOR_COVERAGE _targetname _testrunner _outputname)
        if(CPPCOV_PATH-NOTFOUND)
            message(FATAL_ERROR "OpenCppCoverage not found! Aborting...")
        endif()

        get_target_property(TARGET_TEST_FOLDER ${_testrunner} SOURCE_DIR)  # project_dir/test
        get_filename_component(TARGET_TEST_FOLDER ${TARGET_TEST_FOLDER} DIRECTORY) # project_dir

        # Get configuration directory (Debug/Release etc.), usually emits ${Configuraton}
        get_target_property(BIN_DIR ${_testrunner} RUNTIME_OUTPUT_DIRECTORY)

        # Convert path separators to windows "\" otherwise OpenCppCoverage's source path matching doesn't work
        file(TO_NATIVE_PATH ${TARGET_TEST_FOLDER} WIN_TARGET_TEST_FOLDER)

        set(CPPCOV_ARGS --export_type html:coverage --sources="${WIN_TARGET_TEST_FOLDER}\\src" --sources="${WIN_TARGET_TEST_FOLDER}\\include" --excluded_sources="${WIN_TARGET_TEST_FOLDER}\\tests" --excluded_sources="*.l" --excluded_sources="*.y")

        # Adds a new project to the solution (Not added to build by default)
        # $<CONFIG:Debug> evaluates to 1 when in DEBUG configuration otherwise 0
        # WARNING: Will exclude anything that contains ".l" or ".y" in the file name
        add_custom_target(${_targetname}
            COMMAND rmdir /q /s coverage
            COMMAND if 1==$<CONFIG:Debug> ${CPPCOV_PATH} $<TARGET_FILE:${_testrunner}> ${CPPCOV_ARGS}
            COMMENT "Generating coverage data to /coverage folder:"
        )

        add_dependencies(${_targetname} ${_testrunner})

    endfunction() # SETUP_TARGET_FOR_COVERAGE
endif()
