cmake_minimum_required(VERSION 3.20)
project(properties_app VERSION ${OPENDAQ_EXAMPLES_VERSION})

if(NOT TARGET daq::opendaq) 
    find_package(openDAQ) 
endif()

add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(properties_app properties_app.cpp)
target_link_libraries(properties_app PRIVATE daq::opendaq)
