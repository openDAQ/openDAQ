set_cmake_folder_context(TARGET_FOLDER_NAME)

set(OPENDAQ_EXAMPLES_VERSION ${OPENDAQ_PACKAGE_VERSION})
project(Examples VERSION ${OPENDAQ_EXAMPLES_VERSION} LANGUAGES CXX)

option(APP_ENABLE_WEBPAGE_EXAMPLES "Enable webpage examples" OFF)

if (APP_ENABLE_WEBPAGE_EXAMPLES)
    function(add_optional_device_module_dependencies target_name)
        if (DAQMODULES_OPENDAQ_CLIENT_MODULE)
            if (OPENDAQ_ENABLE_OPCUA)
                add_dependencies(${target_name} daq::opcua_client_module)
            endif()
            if (OPENDAQ_ENABLE_NATIVE_STREAMING)
                add_dependencies(${target_name} daq::native_stream_cl_module)
            endif()
            if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
                add_dependencies(${target_name} daq::ws_stream_cl_module)
            endif()
        endif()
        if (DAQMODULES_REF_DEVICE_MODULE)
            add_dependencies(${target_name} daq::ref_device_module)
        endif()
        if (DAQMODULES_AUDIO_DEVICE_MODULE)
            add_dependencies(${target_name} daq::audio_device_module)
        endif()
    endfunction()

    add_compile_definitions(MODULE_PATH="")

    add_executable(client_discovery cpp/client_discovery/client_discovery.cpp)
    target_link_libraries(client_discovery PRIVATE daq::opendaq)
    add_optional_device_module_dependencies(client_discovery)

    if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND OPENDAQ_ENABLE_OPCUA)
        add_executable(client_local cpp/client_local/client_local.cpp)
        target_link_libraries(client_local PRIVATE daq::opendaq)
        add_dependencies(client_local daq::opcua_client_module)
    else()
        message(WARNING "DAQMODULES_OPENDAQ_CLIENT_MODULE and OPENDAQ_ENABLE_OPCUA must be ON to enable client_local application")
    endif()

    if (DAQMODULES_OPENDAQ_SERVER_MODULE AND OPENDAQ_ENABLE_OPCUA AND DAQMODULES_REF_DEVICE_MODULE)
        add_executable(device_server cpp/device_server/device_server.cpp)
        target_link_libraries(device_server PRIVATE daq::opendaq)
        add_dependencies(device_server daq::opcua_server_module
                                       daq::ref_device_module
        )
        if (OPENDAQ_ENABLE_NATIVE_STREAMING)
            add_dependencies(device_server daq::native_stream_srv_module)
        endif()
        if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
            add_dependencies(device_server daq::ws_stream_srv_module)
        endif()
    else()
        message(WARNING "At least DAQMODULES_OPENDAQ_SERVER_MODULE, OPENDAQ_ENABLE_OPCUA and DAQMODULES_REF_DEVICE_MODULE must be ON to enable device_server application")
    endif()

    add_executable(empty_example cpp/empty_example/empty_example.cpp)
    target_link_libraries(empty_example daq::opendaq)

    if (DAQMODULES_REF_DEVICE_MODULE AND DAQMODULES_REF_FB_MODULE)
        add_executable(function_block_example cpp/function_block_example/function_block_example.cpp)
        target_link_libraries(function_block_example PRIVATE daq::opendaq)
        add_dependencies(function_block_example daq::ref_fb_module
                                                daq::ref_device_module
        )
    else()
        message(WARNING "DAQMODULES_REF_DEVICE_MODULE and DAQMODULES_REF_FB_MODULE must be ON to enable function_block_example application")
    endif()

    if (DAQMODULES_REF_DEVICE_MODULE)
        add_executable(stream_reader_example cpp/stream_reader_example/stream_reader_example.cpp)
        target_link_libraries(stream_reader_example PRIVATE daq::opendaq)
        add_dependencies(stream_reader_example daq::ref_device_module)
    else()
        message(WARNING "DAQMODULES_REF_DEVICE_MODULE must be ON to enable function_block_example application")
    endif()

    add_executable(multi_reader_example cpp/multi_reader_example/multi_reader_example.cpp)
    target_link_libraries(multi_reader_example PRIVATE daq::opendaq)
	
    add_executable(multi_reader_read_same_rates cpp/multi_reader_howto_example/read_same_rates.cpp)
    target_link_libraries(multi_reader_read_same_rates PRIVATE daq::opendaq)
    add_dependencies(multi_reader_read_same_rates daq::ref_device_module)
	
    add_executable(multi_reader_read_different_rates cpp/multi_reader_howto_example/read_different_rates.cpp)
    target_link_libraries(multi_reader_read_different_rates PRIVATE daq::opendaq)
    add_dependencies(multi_reader_read_different_rates daq::ref_device_module)
	
    add_executable(multi_reader_read_same_rates_and_output cpp/multi_reader_howto_example/read_same_rates_and_output.cpp)
    target_link_libraries(multi_reader_read_same_rates_and_output PRIVATE daq::opendaq)
    add_dependencies(multi_reader_read_same_rates_and_output daq::ref_device_module)

    add_executable(reader_basics_example cpp/reader_basics_example/reader_basics_example.cpp)
    target_link_libraries(reader_basics_example PRIVATE daq::opendaq)

    add_executable(reader_timeouts_example cpp/reader_timeouts_example/reader_timeouts_example.cpp)
    target_link_libraries(reader_timeouts_example PRIVATE daq::opendaq)

    add_executable(reader_with_time_stamps cpp/reader_with_time_stamps/reader_with_time_stamps.cpp)
    target_link_libraries(reader_with_time_stamps PRIVATE daq::opendaq)

    add_executable(tail_reader_example cpp/tail_reader_example/tail_reader_example.cpp)
    target_link_libraries(tail_reader_example PRIVATE daq::opendaq)

    add_executable(quick_start_empty cpp/quick_start/quick_start_empty.cpp)
    target_link_libraries(quick_start_empty PRIVATE daq::opendaq)

    if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_executable(client_authentication cpp/client_authentication/client_authentication.cpp)
        target_link_libraries(client_authentication PRIVATE daq::opendaq)
        add_dependencies(client_authentication daq::native_stream_cl_module)
    else()
        message(WARNING "DAQMODULES_OPENDAQ_CLIENT_MODULE and OPENDAQ_ENABLE_NATIVE_STREAMING must be ON to enable client_authentication application")
    endif()

    set(SRC_HeadersConsoleApp cpp/console_application/console_application.h
                              cpp/console_application/app_channel.h
                              cpp/console_application/app_device.h
                              cpp/console_application/app_function_block.h
                              cpp/console_application/app_signal.h
                              cpp/console_application/app_property_object.h
                              cpp/console_application/app_input_port.h
    )

    set(SRC_CppConsoleApp cpp/console_application/console_application.cpp
                          cpp/console_application/main.cpp
                          cpp/console_application/app_channel.cpp
                          cpp/console_application/app_device.cpp
                          cpp/console_application/app_signal.cpp
                          cpp/console_application/app_function_block.cpp
                          cpp/console_application/app_property_object.cpp
                          cpp/console_application/app_input_port.cpp
    )
    add_executable(console_application ${SRC_HeadersConsoleApp}
                                       ${SRC_CppConsoleApp}
    )
    target_link_libraries(console_application PRIVATE daq::opendaq)
    add_optional_device_module_dependencies(console_application)
    if (DAQMODULES_REF_FB_MODULE)
        add_dependencies(console_application daq::ref_fb_module)
    endif()

    if (DAQMODULES_OPENDAQ_SERVER_MODULE AND OPENDAQ_ENABLE_OPCUA AND DAQMODULES_REF_DEVICE_MODULE)
        add_executable(quick_start_simulator cpp/quick_start/quick_start_simulator.cpp)
        target_link_libraries(quick_start_simulator PRIVATE daq::opendaq)
        add_dependencies(quick_start_simulator daq::opcua_server_module
                                               daq::ref_device_module
        )
        if (OPENDAQ_ENABLE_NATIVE_STREAMING)
            add_dependencies(quick_start_simulator daq::native_stream_srv_module)
        endif()
        if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
            add_dependencies(quick_start_simulator daq::ws_stream_srv_module)
        endif()
    else()
        message(WARNING "At least DAQMODULES_OPENDAQ_SERVER_MODULE, OPENDAQ_ENABLE_OPCUA and DAQMODULES_REF_DEVICE_MODULE must be ON to enable quick_start_simulator application")
    endif()

    if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND DAQMODULES_REF_FB_MODULE)
        if (OPENDAQ_ENABLE_NATIVE_STREAMING OR OPENDAQ_ENABLE_OPCUA)
            add_executable(quick_start_full cpp/quick_start/quick_start_full.cpp)
            target_link_libraries(quick_start_full PRIVATE daq::opendaq)
            add_dependencies(quick_start_full daq::ref_fb_module)
            if (OPENDAQ_ENABLE_OPCUA)
                add_dependencies(quick_start_full daq::opcua_client_module)
            endif()
            if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
                add_dependencies(quick_start_full daq::ws_stream_cl_module)
            endif()
            if (OPENDAQ_ENABLE_NATIVE_STREAMING)
                add_dependencies(quick_start_full daq::native_stream_cl_module)
            endif()
        else()
            message(WARNING "OPENDAQ_ENABLE_NATIVE_STREAMING or OPENDAQ_ENABLE_OPCUA must be ON to enable quick_start_full application")
        endif()

        if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
            add_executable(quick_start_websocket cpp/quick_start/quick_start_websocket.cpp)
            target_link_libraries(quick_start_websocket PRIVATE daq::opendaq)
            add_dependencies(quick_start_websocket daq::ws_stream_cl_module
                                                   daq::ref_fb_module
            )
        else()
            message(WARNING "OPENDAQ_ENABLE_WEBSOCKET_STREAMING must be ON to enable quick_start_websocket applications")
        endif()

        if (OPENDAQ_ENABLE_NATIVE_STREAMING)
            add_executable(quick_start_native cpp/quick_start/quick_start_native.cpp)
            target_link_libraries(quick_start_native PRIVATE daq::opendaq)
            add_dependencies(quick_start_native daq::native_stream_cl_module
                                                daq::ref_fb_module
            )
        else()
            message(WARNING "OPENDAQ_ENABLE_NATIVE_STREAMING must be ON to enable quick_start_native applications")
        endif()
    else()
        message(WARNING "DAQMODULES_OPENDAQ_CLIENT_MODULE and DAQMODULES_REF_FB_MODULE must be ON to enable quick_start_* applications")
    endif()

    if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND
        DAQMODULES_OPENDAQ_SERVER_MODULE AND
        OPENDAQ_ENABLE_WEBSOCKET_STREAMING AND
        OPENDAQ_ENABLE_OPCUA AND
        DAQMODULES_REF_DEVICE_MODULE AND
        DAQMODULES_REF_FB_MODULE)
        add_executable(csv_writer cpp/csv_writer/csv_writer.cpp)
        target_link_libraries(csv_writer PRIVATE daq::opendaq)
        add_dependencies(csv_writer daq::opcua_server_module
                                    daq::opcua_client_module
                                    daq::ref_fb_module
                                    daq::ref_device_module
                                    daq::ws_stream_cl_module
                                    daq::ws_stream_srv_module
            )
    else()
        message(WARNING "DAQMODULES_OPENDAQ_SERVER_MODULE, DAQMODULES_OPENDAQ_CLIENT_MODULE, OPENDAQ_ENABLE_WEBSOCKET_STREAMING, OPENDAQ_ENABLE_OPCUA, DAQMODULES_REF_FB_MODULE and DAQMODULES_REF_DEVICE_MODULE must be ON to enable csv_writer application")
    endif()

    if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND OPENDAQ_ENABLE_NATIVE_STREAMING AND OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        add_executable(streaming_configuration_example cpp/streaming_configuration_example/streaming_configuration_example.cpp)
        target_link_libraries(streaming_configuration_example PRIVATE daq::opendaq)
        if (DAQMODULES_OPENDAQ_CLIENT_MODULE AND OPENDAQ_ENABLE_OPCUA)
            add_dependencies(streaming_configuration_example daq::opcua_client_module)
        endif()
        add_dependencies(streaming_configuration_example daq::ws_stream_cl_module
                                                         daq::native_stream_cl_module)
    else()
        message(WARNING "At least DAQMODULES_OPENDAQ_CLIENT_MODULE, OPENDAQ_ENABLE_WEBSOCKET_STREAMING and OPENDAQ_ENABLE_NATIVE_STREAMING must be ON to enable streaming_configuration_example application")
    endif()

    if (DAQMODULES_OPENDAQ_SERVER_MODULE AND DAQMODULES_OPENDAQ_CLIENT_MODULE AND OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_executable(reconnection_example cpp/reconnection_example/reconnection.cpp)
        target_link_libraries(reconnection_example PRIVATE daq::opendaq)
        add_dependencies(reconnection_example daq::native_stream_cl_module
                                              daq::native_stream_srv_module
        )

        add_executable(reconnection_example_v3_10 cpp/reconnection_example/reconnection_v3_10.cpp)
        target_link_libraries(reconnection_example_v3_10 PRIVATE daq::opendaq)
        add_dependencies(reconnection_example_v3_10 daq::native_stream_cl_module
                                                    daq::native_stream_srv_module
        )
    else()
        message(WARNING "DAQMODULES_OPENDAQ_SERVER_MODULE, DAQMODULES_OPENDAQ_CLIENT_MODULE and OPENDAQ_ENABLE_NATIVE_STREAMING must be ON to enable reconnection_example applications")
    endif()
endif()
