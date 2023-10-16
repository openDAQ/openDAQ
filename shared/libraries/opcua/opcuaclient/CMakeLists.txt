cmake_minimum_required(VERSION 3.2)
set_cmake_folder_context(TARGET_FOLDER_NAME client)
project(OpcUaClient CXX)

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
