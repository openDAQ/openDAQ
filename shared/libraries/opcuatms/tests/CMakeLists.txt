cmake_minimum_required(VERSION 3.5)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(opcuatms_integration_tests CXX)

add_subdirectory(test_utils)

if (OPENDAQ_ENABLE_INTEGRATION_TESTS)
    add_subdirectory(opcuatms_integration)
endif()
