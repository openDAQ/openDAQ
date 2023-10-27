set_cmake_folder_context(TARGET_FOLDER_NAME)

add_subdirectory(core_types)
add_subdirectory(core_objects)
add_subdirectory(opendaq)

add_subdirectory(py_opendaq_daq)

if(OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS)
    add_subdirectory(tests)
endif()
