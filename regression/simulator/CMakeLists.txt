project(regression_simulator)
add_executable(regression_simulator regression_simulator.cpp)
target_link_libraries(regression_simulator PRIVATE daq::opendaq)
target_link_options(regression_simulator PRIVATE -Wl,--enable-new-dtags -Wl,-rpath,.)
