cmake_minimum_required(VERSION 3.20)
project(quick_start_empty VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_executable(quick_start_empty quick_start_empty.cpp)
target_link_libraries(quick_start_empty PRIVATE daq::opendaq)

add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

option(OPENDAQ_DEVICE_SIMULATOR "Creates a device simulator without a discovery server." ON)
option(OPENDAQ_BUILD_FULL_EXAMPLE "Full final tutorial example code" ON)
option(OPENDAQ_BUILD_WEBSOCKET_EXAMPLE "Websocket streaming device example code" ON)
option(OPENDAQ_BUILD_NATIVE_STREAMING_EXAMPLE "Native streaming device example code" ON)

if (OPENDAQ_CREATE_DEVICE_SIMULATOR)
  project(quick_start_simulator VERSION ${OPENDAQ_EXAMPLES_VERSION})
  add_executable(quick_start_simulator quick_start_simulator.cpp)
  target_link_libraries(quick_start_simulator PRIVATE daq::opendaq)
endif()

if (OPENDAQ_BUILD_FULL_EXAMPLE)
  project(quick_start_full VERSION ${OPENDAQ_EXAMPLES_VERSION})
  add_executable(quick_start_full quick_start_full.cpp)
  target_link_libraries(quick_start_full PRIVATE daq::opendaq)
endif()

if (OPENDAQ_BUILD_WEBSOCKET_EXAMPLE)
  project(quick_start_websocket VERSION ${OPENDAQ_EXAMPLES_VERSION})
  add_executable(quick_start_websocket quick_start_websocket.cpp)
  target_link_libraries(quick_start_websocket PRIVATE daq::opendaq)
endif()

if (OPENDAQ_BUILD_NATIVE_STREAMING_EXAMPLE)
  project(quick_start_native VERSION ${OPENDAQ_EXAMPLES_VERSION})
  add_executable(quick_start_native quick_start_native.cpp)
  target_link_libraries(quick_start_native PRIVATE daq::opendaq)
endif()
