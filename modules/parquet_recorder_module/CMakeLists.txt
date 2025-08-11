cmake_minimum_required(VERSION 4.0)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(ParquetRecorderModule VERSION ${OPENDAQ_PACKAGE_VERSION} LANGUAGES C CXX)

add_subdirectory(src)

if(OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
