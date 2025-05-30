set_cmake_folder_context(TARGET_FOLDER_NAME)

if (MSVC)
    add_compile_options(/wd4100)
endif()

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

if (DAQSIMULATOR_ENABLE_SIMULATOR_APP)
    message(NOTICE "Enable simulator support in reference device module.")
    add_compile_definitions(DAQMODULES_REF_DEVICE_MODULE_SIMULATOR_ENABLED)
endif()

if (DAQMODULES_OPENDAQ_CLIENT_MODULE)
    message(STATUS "Client modules:")

    if (OPENDAQ_ENABLE_OPCUA)
        message(STATUS "    - OpcUa client module")
        add_subdirectory(opcua_client_module)
    endif()

    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        message(STATUS "    - Websocket client module")
        add_subdirectory(websocket_streaming_client_module)
    endif()

    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        message(STATUS "    - Native streaming client module")
        add_subdirectory(native_streaming_client_module)
    endif()
endif()

if (DAQMODULES_OPENDAQ_SERVER_MODULE)
    message(STATUS "Server modules:")

    if (OPENDAQ_ENABLE_OPCUA)
        message(STATUS "    - OpcUa server module")
        add_subdirectory(opcua_server_module)
    endif()

    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        message(STATUS "    - Websocket server module")
        add_subdirectory(websocket_streaming_server_module)
        add_subdirectory(new_websocket_streaming_server_module)
    endif()

    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        message(STATUS "    - Native streaming server module")
        add_subdirectory(native_streaming_server_module)
    endif()
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

if (DAQMODULES_AUDIO_DEVICE_MODULE)
    message(STATUS "Audio device module")
    add_subdirectory(audio_device_module)
endif()

if (DAQMODULES_BASIC_CSV_RECORDER_MODULE)
    message(STATUS "Basic CSV recorder module")
    add_subdirectory(basic_csv_recorder_module)
endif()

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
