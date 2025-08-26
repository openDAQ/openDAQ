cmake_minimum_required(VERSION 3.20)

file(READ "../cpp/opendaq_examples_version" examples_version)
string(STRIP ${examples_version} examples_version)

string(REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" examples_version_major_minor_patch "${examples_version}")

set(OPENDAQ_EXAMPLES_VERSION "${examples_version_major_minor_patch}")

project(c_application_tutorial VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_executable(c_application_tutorial application_tutorial.c)
target_link_libraries(c_application_tutorial PRIVATE daq::copendaq)

add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

option(OPENDAQ_C_CREATE_EMPTY_EXAMPLE "Creates an empty application example." ON)

if(OPENDAQ_C_CREATE_EMPTY_EXAMPLE)
    project(c_application_empty VERSION ${OPENDAQ_EXAMPLES_VERSION})
    add_executable(c_application_empty application_empty.c)
    target_link_libraries(c_application_empty PRIVATE daq::copendaq)
endif()
