cmake_minimum_required(VERSION 3.20)
project(tail_reader_example
        VERSION ${OPENDAQ_EXAMPLES_VERSION}
        LANGUAGES CXX
)

find_package(openDAQ)

add_executable(tail_reader_example tail_reader_example.cpp)
target_link_libraries(tail_reader_example PRIVATE daq::opendaq)
