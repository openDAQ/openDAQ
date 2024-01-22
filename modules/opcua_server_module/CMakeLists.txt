cmake_minimum_required(VERSION 3.5)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(ServerModule VERSION 2.0.0 LANGUAGES CXX)

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
