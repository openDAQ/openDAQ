opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)

if (MSVC)
    add_compile_options(/wd4100)
endif()

if (DAQMODULES_OPENDAQ_CLIENT_MODULE)
    message(STATUS "Client modules:")

    if (OPENDAQ_ENABLE_OPCUA)
        message(STATUS "    - OpcUa client module")
    endif()

    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        message(STATUS "    - Websocket client module (LT streaming)")
    endif()

    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        message(STATUS "    - Native client module (streaming and configuration)")
        add_subdirectory(native_streaming_client_module)
    endif()
endif()

if (DAQMODULES_OPENDAQ_SERVER_MODULE)
    message(STATUS "Server modules:")

    if (OPENDAQ_ENABLE_OPCUA)
        message(STATUS "    - OpcUa server module")
    endif()

    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        message(STATUS "    - Websocket server module (LT streaming)")
    endif()

    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        message(STATUS "    - Native server module (streaming and configuration)")
        add_subdirectory(native_streaming_server_module)
    endif()
endif()
