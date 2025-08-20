cmake_minimum_required(VERSION 3.20)
project(reader_with_time_stamps VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)

add_executable(reader_with_time_stamps reader_with_time_stamps.cpp)
target_link_libraries(reader_with_time_stamps PRIVATE daq::opendaq)
