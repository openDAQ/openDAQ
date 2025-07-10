project(regression_simulator)
add_executable(regression_simulator regression_simulator.cpp)
target_link_libraries(regression_simulator PRIVATE daq::opendaq)

if(NOT APPLE)
    target_link_options(regression_simulator PRIVATE -Wl,--enable-new-dtags)
endif()
target_link_options(regression_simulator PRIVATE -Wl,-rpath,.)
