cmake_minimum_required(VERSION 3.10)
set_cmake_folder_context(TARGET_FOLDER_NAME server)
project(OpcUaServer CXX)

if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
