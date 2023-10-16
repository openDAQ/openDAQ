set_cmake_folder_context(TARGET_FOLDER_NAME)

opendaq_dependency(
    NAME                pybind11
    REQUIRED_VERSION    2.10.4
    GIT_REPOSITORY      https://github.com/pybind/pybind11
    GIT_REF             v2.10.4
    EXPECT_COMMAND      pybind11_add_module
)

add_subdirectory(core_types)
add_subdirectory(core_objects)
add_subdirectory(opendaq)

add_subdirectory(py_opendaq_daq)

if(OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS)
    add_subdirectory(tests)
endif()
