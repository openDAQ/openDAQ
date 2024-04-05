set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME signal)

set(TEST_HEADERS
)

set(TEST_SOURCES
    test_connection.cpp
    test_data_rules.cpp
    test_dimension.cpp
    test_dimension_rules.cpp
    test_factories.cpp
    test_input_port.cpp
    test_packet_e2e.cpp
    test_scaling.cpp
    test_signal.cpp
	test_struct_descriptor.cpp
    test_data_descriptor.cpp
    test_data_packet.cpp
	test_event_packet.cpp
    test_signal_container.cpp
    test_deleter.cpp
    test_binary_packet.cpp
    test_allocated_packets.cpp
    test_malloc.cpp
    test_external_alloc.cpp
    test_range.cpp
    test_packet_destruct_callback.cpp
    test_signal_event_packets.cpp
    test_data_path.cpp
)

if (OPENDAQ_MIMALLOC_SUPPORT)
    list(APPEND TEST_SOURCES test_mimalloc.cpp)
endif()

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

target_link_libraries(${TEST_APP} PRIVATE daq::opendaq_mocks
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
