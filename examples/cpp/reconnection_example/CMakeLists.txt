cmake_minimum_required(VERSION 3.20)

project(reconnection VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)

add_executable(reconnection reconnection.cpp)
target_link_libraries(reconnection PRIVATE daq::opendaq)

project(reconnection_v3_10 VERSION ${OPENDAQ_EXAMPLES_VERSION})
add_executable(reconnection_v3_10 reconnection_v3_10.cpp)
target_link_libraries(reconnection_v3_10 PRIVATE daq::opendaq)
