cmake_minimum_required(VERSION 3.20)
project(reader_timeouts_example
        VERSION ${OPENDAQ_EXAMPLES_VERSION}
        LANGUAGES CXX
)

find_package(openDAQ)

add_executable(reader_timeouts_example reader_timeouts_example.cpp)
target_link_libraries(reader_timeouts_example PRIVATE daq::opendaq)
