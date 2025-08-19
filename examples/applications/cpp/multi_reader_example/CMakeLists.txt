cmake_minimum_required(VERSION 3.20)
project(multi_reader_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(multi_reader_example multi_reader_example.cpp)
target_link_libraries(multi_reader_example PRIVATE daq::opendaq)
