cmake_minimum_required(VERSION 3.20)
project(client_local VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(client_local client_local.cpp)
target_link_libraries(client_local PRIVATE daq::opendaq)