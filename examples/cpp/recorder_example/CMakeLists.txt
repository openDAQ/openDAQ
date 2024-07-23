cmake_minimum_required(VERSION 3.20)
project(recorder_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(recorder_example recorder_example.cpp)
target_link_libraries(recorder_example PRIVATE daq::opendaq)