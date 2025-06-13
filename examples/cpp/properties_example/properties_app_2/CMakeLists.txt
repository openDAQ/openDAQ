cmake_minimum_required(VERSION 3.20)
project(properties_app_2 VERSION ${OPENDAQ_EXAMPLES_VERSION})

if(NOT TARGET daq::opendaq) 
    find_package(openDAQ) 
endif()

add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

include_directories("../common")

add_executable(properties_app_2 properties_app_2.cpp "../common/utils.h")
target_link_libraries(properties_app_2 PRIVATE daq::opendaq)
set_target_properties(properties_app_2 PROPERTIES FOLDER ${PROPERTIES_FOLDER_PATH})

if (MSVC)
    target_compile_options(properties_app_2 PRIVATE /bigobj)
endif()
