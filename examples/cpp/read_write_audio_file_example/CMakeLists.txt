cmake_minimum_required(VERSION 3.20)
project(audio_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(audio_example audio_example.cpp)
target_link_libraries(audio_example PRIVATE daq::opendaq)