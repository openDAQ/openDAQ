# MultiReader2 trial applications: discover and configure devices, read them with MultiReader2 and record to CSV
foreach(app mr2_probe mr2_local_bench mr2_network_rates mr2_rpi_adc)
    add_executable(${app} ${app}.cpp trial_common.h)
    target_link_libraries(${app} PRIVATE daq::opendaq)
    add_optional_device_module_dependencies(${app})
endforeach()
