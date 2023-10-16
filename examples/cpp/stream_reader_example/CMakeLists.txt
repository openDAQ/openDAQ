cmake_minimum_required(VERSION 3.20)
project(stream_reader_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(stream_reader_example stream_reader_example.cpp)
target_link_libraries(stream_reader_example PRIVATE daq::opendaq)