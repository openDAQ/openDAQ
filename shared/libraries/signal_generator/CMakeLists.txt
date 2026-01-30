cmake_minimum_required(VERSION 3.13)
opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)
project(signal_generator CXX)

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
