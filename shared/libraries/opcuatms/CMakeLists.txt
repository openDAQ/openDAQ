set_cmake_folder_context(TARGET_FOLDER_NAME)

add_subdirectory(opcuatms)
add_subdirectory(opcuatms_server)
add_subdirectory(opcuatms_client)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
