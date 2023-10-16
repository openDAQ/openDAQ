cmake_minimum_required(VERSION 3.20)
project(reader_basics_example
        VERSION ${OPENDAQ_EXAMPLES_VERSION}
        LANGUAGES CXX
)

find_package(openDAQ)

add_executable(reader_basics_example reader_basics_example.cpp)
target_link_libraries(reader_basics_example PRIVATE daq::opendaq)
