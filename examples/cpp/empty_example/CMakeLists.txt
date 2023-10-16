cmake_minimum_required(VERSION 3.20)
project(empty_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(empty_example empty_example.cpp)
target_link_libraries(empty_example PRIVATE daq::opendaq)