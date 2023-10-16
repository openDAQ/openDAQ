cmake_minimum_required(VERSION 3.2)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(BBTestUtils CXX)

if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()

add_subdirectory(src)
