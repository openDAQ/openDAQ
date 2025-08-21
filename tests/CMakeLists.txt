cmake_minimum_required(VERSION 3.10)
set_cmake_folder_context(TARGET_FOLDER_NAME)

add_subdirectory(integration)

if (OPENDAQ_ENABLE_REGRESSION_TESTS)
    add_subdirectory(regression)
endif()
