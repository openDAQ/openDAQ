set_cmake_context()

add_subdirectory(utils)
add_subdirectory(signal_generator)
add_subdirectory(discovery_common)
add_subdirectory(discovery)
add_subdirectory(discovery_server)

if (OPENDAQ_ENABLE_TEST_UTILS)
    add_subdirectory(testutils)
endif()

if (OPENDAQ_ENABLE_OPCUA)
    message(STATUS "OPC-UA")
    add_subdirectory(opcua)
    add_subdirectory(opcuatms)
endif()

if (OPENDAQ_ENABLE_NATIVE_STREAMING)
    message(STATUS "Native streaming")
    add_subdirectory(native_streaming_protocol)
    add_subdirectory(packet_streaming)
    add_subdirectory(config_protocol)
endif()
