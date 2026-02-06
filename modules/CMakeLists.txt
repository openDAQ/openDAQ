opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)

if (MSVC)
    add_compile_options(/wd4100)
endif()

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

if (DAQMODULES_OPENDAQ_CLIENT_MODULE)
    message(STATUS "Client modules:")

    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        message(STATUS "    - Native streaming client module")
        add_subdirectory(native_streaming_client_module)
    endif()
endif()

if (DAQMODULES_OPENDAQ_SERVER_MODULE)
    message(STATUS "Server modules:")

    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        message(STATUS "    - Native streaming server module")
        add_subdirectory(native_streaming_server_module)
    endif()
endif()

if (OPENDAQ_ENABLE_OPCUA AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    message(STATUS "OpcUa modules")
    add_subdirectory(opcua_modules)
endif()

if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING AND (DAQMODULES_OPENDAQ_CLIENT_MODULE OR DAQMODULES_OPENDAQ_SERVER_MODULE))
    message(STATUS "Websocket modules")
    add_subdirectory(websocket_streaming_modules)
endif()
