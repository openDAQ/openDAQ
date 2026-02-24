cmake_minimum_required(VERSION 3.24)
opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME licensing_example)

project(LicensingExample CXX)

add_subdirectory(src)

target_compile_definitions(licensing_example PUBLIC RESOURCE_PATH="${CMAKE_CURRENT_LIST_DIR}/resources")
