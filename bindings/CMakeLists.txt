set_cmake_folder_context(TARGET_FOLDER_NAME)

if (OPENDAQ_GENERATE_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

if (OPENDAQ_GENERATE_CSHARP_BINDINGS)
    add_subdirectory(dotnet)
endif()
