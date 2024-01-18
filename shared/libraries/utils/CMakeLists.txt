cmake_minimum_required(VERSION 3.5)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(${SDK_TARGET_NAME}_utils CXX)

add_subdirectory(src EXCLUDE_FROM_ALL)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
