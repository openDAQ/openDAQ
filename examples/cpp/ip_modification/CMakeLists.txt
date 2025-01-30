cmake_minimum_required(VERSION 3.20)
project(ip_modification VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(ip_modification ip_modification.cpp)
target_link_libraries(ip_modification PRIVATE daq::opendaq)

