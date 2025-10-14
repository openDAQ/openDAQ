set_cmake_folder_context(TARGET_FOLDER_NAME)

if (MSVC)
    add_compile_options(/wd4100)

    # loss of data / precision, unsigned <--> signed
    #
    # 'argument' : conversion from 'type1' to 'type2', possible loss of data
    # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4244
    add_compile_options(/wd4244)

    # 'var' : conversion from 'size_t' to 'type', possible loss of data
    # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267
    add_compile_options(/wd4267)
endif()

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

if (DAQMODULES_LICENSING_MODULE)
		message(STATUS "    - Reference licensing implementation module")
        add_subdirectory(licensing_module)
endif()

if (DAQSIMULATOR_ENABLE_SIMULATOR_APP)
    message(NOTICE "Enable simulator support in reference device module.")
    add_compile_definitions(DAQMODULES_REF_DEVICE_MODULE_SIMULATOR_ENABLED)
endif()

if (DAQMODULES_EMPTY_MODULE)
    message(STATUS "Empty module")
    add_subdirectory(empty_module)
endif()

if (DAQMODULES_REF_DEVICE_MODULE)
    message(STATUS "Reference device module")
    add_subdirectory(ref_device_module)
endif()

if (DAQMODULES_REF_FB_MODULE)
    message(STATUS "Reference function block module")
    add_subdirectory(ref_fb_module)
endif()

if (DAQMODULES_SIMULATOR_DEVICE_MODULE)
    message(STATUS "Simulator device module")
    add_subdirectory(simulator_device_module)
endif()

if (DAQMODULES_AUDIO_DEVICE_MODULE)
    message(STATUS "Audio device module")
    add_subdirectory(audio_device_module)
endif()

if (DAQMODULES_BASIC_CSV_RECORDER_MODULE)
    message(STATUS "Basic CSV recorder module")
    add_subdirectory(basic_csv_recorder_module)
endif()

if (DAQMODULES_PARQUET_RECORDER_MODULE)
    message(STATUS "Parquet recorder module")
    if (CMAKE_VERSION VERSION_LESS "4.0.0")
        message(FATAL_ERROR "Parquet recorder module requires CMake 4.0.0 or higher.")
    endif()
    add_subdirectory(parquet_recorder_module)
endif()

if (DAQMODULES_PROPERTIES_MODULE)
    message(STATUS "Properties example module")
    add_subdirectory(property_system_example_module)
endif()
