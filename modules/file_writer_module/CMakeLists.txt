cmake_minimum_required(VERSION 3.2)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(FileWriterModule VERSION 0.0.1 LANGUAGES CXX)

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
