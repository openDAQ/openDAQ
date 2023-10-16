set(APP_SIMULATOR application_simulator)

set(SRC_Cpp main.cpp
)

add_executable(${APP_SIMULATOR} ${SRC_Cpp}
)

target_include_directories(${APP_SIMULATOR} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
                                                   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/>
)

target_link_libraries(${APP_SIMULATOR} PRIVATE daq::opendaq
)
