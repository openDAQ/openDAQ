set_cmake_folder_context(TARGET_FOLDER_NAME)

add_subdirectory(core_types)
add_subdirectory(core_objects)
add_subdirectory(opendaq)

add_subdirectory(py_opendaq_daq)

option(OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS_ONLY "Enable only the python bindings tests" OFF)

if(OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS OR OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS_ONLY)
    add_subdirectory(tests)
endif()
