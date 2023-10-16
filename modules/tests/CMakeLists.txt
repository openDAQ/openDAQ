if (DAQMODULES_REF_FB_MODULE AND DAQMODULES_REF_DEVICE_MODULE)
    message(STATUS "Reference device & function block module test app")
    add_subdirectory(test_ref_modules)
endif()

if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND DAQMODULES_OPENDAQ_SERVER_MODULE AND DAQMODULES_REF_FB_MODULE AND DAQMODULES_REF_DEVICE_MODULE)
    message(STATUS "${SDK_NAME} device and server module test app")
    add_subdirectory(test_opendaq_device_modules)
endif()
