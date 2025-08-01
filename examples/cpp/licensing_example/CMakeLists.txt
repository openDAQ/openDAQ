cmake_minimum_required(VERSION 3.20)
project(licensing_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(licensing_example licensing_example.cpp)
target_link_libraries(licensing_example PRIVATE daq::opendaq)

target_compile_definitions(licensing_example PUBLIC RESOURCE_PATH="${CMAKE_CURRENT_LIST_DIR}")
