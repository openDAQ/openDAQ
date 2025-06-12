cmake_minimum_required(VERSION 3.10)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(opcuatms_server VERSION ${OPENDAQ_PACKAGE_VERSION} LANGUAGES CXX)

add_subdirectory(src)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
