cmake_minimum_required(VERSION 3.20)
project(csv_writer VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(Threads)

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(csv_writer csv_writer.cpp)
target_link_libraries(csv_writer PRIVATE daq::opendaq
                                         Threads::Threads
)
