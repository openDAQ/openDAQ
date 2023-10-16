cmake_minimum_required(VERSION 3.20)
project(streaming_configuration_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(streaming_configuration_example streaming_configuration_example.cpp)
target_link_libraries(streaming_configuration_example PRIVATE daq::opendaq)