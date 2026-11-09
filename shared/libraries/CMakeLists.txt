opendaq_set_cmake_context()

add_subdirectory(utils)
add_subdirectory(discovery_common)
add_subdirectory(discovery)
add_subdirectory(discovery_server)

if (OPENDAQ_ENABLE_TEST_UTILS)
    # signal_generator is consumed only by the mocks, which live behind the same option
    add_subdirectory(signal_generator)
    add_subdirectory(testutils)
endif()

if (OPENDAQ_ENABLE_NATIVE_STREAMING)
    message(STATUS "Native streaming")
    add_subdirectory(native_streaming_protocol)
    add_subdirectory(packet_streaming)
    add_subdirectory(config_protocol)
endif()

# Built when at least one SFTP backend is enabled.
if (OPENDAQ_ENABLE_SFTP_FILE_TRANSFER_LIBSSH2)
    add_subdirectory(sftp_file_transfer)
endif()
