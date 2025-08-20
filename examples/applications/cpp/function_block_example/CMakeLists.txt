cmake_minimum_required(VERSION 3.20)
project(function_block_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(function_block_example function_block_example.cpp)
target_link_libraries(function_block_example PRIVATE daq::opendaq)