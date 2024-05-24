project(regression_simulator)
set(CMAKE_INSTALL_RPATH $ORIGIN) #TODO: does this help?
add_executable(regression_simulator regression_simulator.cpp)
target_link_libraries(regression_simulator PRIVATE daq::opendaq)
