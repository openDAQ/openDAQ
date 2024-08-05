set_cmake_folder_context(TARGET_FOLDER_NAME)

set(OPENDAQ_EXAMPLES_VERSION 3.2.3)
project(Examples VERSION ${OPENDAQ_EXAMPLES_VERSION} LANGUAGES CXX)

option(APP_ENABLE_WEBPAGE_EXAMPLES "Enable webpage examples" OFF)

if (APP_ENABLE_WEBPAGE_EXAMPLES)
    add_compile_definitions(MODULE_PATH="")

    add_executable(client_discovery cpp/client_discovery/client_discovery.cpp)
    target_link_libraries(client_discovery PRIVATE daq::opendaq)
    add_dependencies(client_discovery daq::opcua_client_module)
    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_dependencies(client_discovery daq::native_stream_cl_module)
    endif()
    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        add_dependencies(client_discovery daq::ws_stream_cl_module)
    endif()

    add_executable(client_local cpp/client_local/client_local.cpp)
    target_link_libraries(client_local PRIVATE daq::opendaq)
    add_dependencies(client_local daq::opcua_client_module)
    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_dependencies(client_local daq::native_stream_cl_module)
    endif()
    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        add_dependencies(client_local daq::ws_stream_cl_module)
    endif()

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

    add_executable(empty_example cpp/empty_example/empty_example.cpp)
    target_link_libraries(empty_example daq::opendaq)

    add_executable(function_block_example cpp/function_block_example/function_block_example.cpp)
    target_link_libraries(function_block_example PRIVATE daq::opendaq)
    add_dependencies(function_block_example daq::ref_fb_module
                                            daq::ref_device_module
    )

    add_executable(stream_reader_example cpp/stream_reader_example/stream_reader_example.cpp)
    target_link_libraries(stream_reader_example PRIVATE daq::opendaq)
    add_dependencies(stream_reader_example daq::ref_device_module)

    add_executable(multi_reader_example cpp/multi_reader_example/multi_reader_example.cpp)
    target_link_libraries(multi_reader_example PRIVATE daq::opendaq)

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
    add_dependencies(console_application daq::opcua_server_module
                                         daq::opcua_client_module
                                         daq::ref_fb_module
                                         daq::ref_device_module
    )
    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        add_dependencies(console_application daq::ws_stream_cl_module
                                             daq::ws_stream_srv_module
        )
    endif()
    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_dependencies(console_application daq::native_stream_cl_module
                                             daq::native_stream_srv_module
        )
    endif()

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

    add_executable(quick_start_full cpp/quick_start/quick_start_full.cpp)
    target_link_libraries(quick_start_full PRIVATE daq::opendaq)
    add_dependencies(quick_start_full daq::opcua_client_module
                                      daq::ref_fb_module
    )
    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        add_dependencies(quick_start_full daq::ws_stream_cl_module)
    endif()
    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_dependencies(quick_start_full daq::native_stream_cl_module)
    endif()

    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        add_executable(quick_start_websocket cpp/quick_start/quick_start_websocket.cpp)
        target_link_libraries(quick_start_websocket PRIVATE daq::opendaq)
        add_dependencies(quick_start_websocket daq::ws_stream_cl_module
                                               daq::ref_fb_module
        )
    endif()

    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_executable(quick_start_native cpp/quick_start/quick_start_native.cpp)
        target_link_libraries(quick_start_native PRIVATE daq::opendaq)
        add_dependencies(quick_start_native daq::native_stream_cl_module
                                            daq::ref_fb_module
        )
    endif()

    add_executable(csv_writer cpp/csv_writer/csv_writer.cpp)
    target_link_libraries(csv_writer PRIVATE daq::opendaq)
    add_dependencies(csv_writer daq::opcua_server_module
                                daq::opcua_client_module
                                daq::ref_fb_module
                                daq::ref_device_module
    )
    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        add_dependencies(csv_writer daq::ws_stream_cl_module
                                    daq::ws_stream_srv_module
        )
    endif()
    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_dependencies(csv_writer daq::native_stream_cl_module
                                    daq::native_stream_srv_module
        )
    endif()

    add_executable(streaming_configuration_example cpp/streaming_configuration_example/streaming_configuration_example.cpp)
    target_link_libraries(streaming_configuration_example PRIVATE daq::opendaq)
    add_dependencies(streaming_configuration_example daq::opcua_client_module
                                                     daq::ref_fb_module
    )
    if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
        add_dependencies(streaming_configuration_example daq::ws_stream_cl_module)
    endif()
    if (OPENDAQ_ENABLE_NATIVE_STREAMING)
        add_dependencies(streaming_configuration_example daq::native_stream_cl_module)
    endif()
endif()
