set(APP_EXAMPLE opendaq_ref_modules_example)

add_executable(${APP_EXAMPLE} ref_modules_example.cpp)

target_link_libraries(${APP_EXAMPLE} PRIVATE daq::ref_fb_module
                                             daq::ref_device_module
)

