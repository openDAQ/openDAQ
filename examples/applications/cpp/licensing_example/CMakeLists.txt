cmake_minimum_required(VERSION 3.20)
project(licensing_example VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

add_executable(licensing_example licensing_example.cpp)


if (MSVC)
    target_compile_options(${LIB_NAME} 
        PRIVATE 
            /bigobj 
            /wd4100     #Avoid 'unreferenced formal parameter' compiler warning  (Otherwise will not compile in VS in Release build...)
            /ZI         #Enable Hot Reload per default (see: ) https://learn.microsoft.com/en-us/visualstudio/debugger/edit-and-continue-visual-cpp?view=vs-2022#BKMK_Enable_or_disable_automatic_invocation_of_Edit_and_Continue
    )
endif()

target_link_libraries(licensing_example PRIVATE daq::opendaq ImageHlp)
target_link_options(your_target PRIVATE /VERBOSE:LIB)

target_compile_definitions(licensing_example PUBLIC RESOURCE_PATH="${CMAKE_CURRENT_LIST_DIR}")
