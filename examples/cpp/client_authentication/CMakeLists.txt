cmake_minimum_required(VERSION 3.20)
project(client_authentication VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(client_authentication client_authentication.cpp)
target_link_libraries(client_authentication PRIVATE daq::opendaq)