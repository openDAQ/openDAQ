set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME reader)

set(TEST_HEADERS reader_common.h
)

set(TEST_SOURCES test_factories.cpp
                 test_tail_reader.cpp
                 test_packet_reader.cpp
                 test_stream_reader.cpp
                 test_date.cpp
                 test_block_reader.cpp
                 test_time_reader.cpp
                 test_multi_reader.cpp
                 test_stream_reader_from_input_port.cpp
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

if (MSVC)
    set_source_files_properties(
        test_stream_reader.cpp
        PROPERTIES
            COMPILE_FLAGS /wd4309 # truncation of constant value
    )

    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

target_link_libraries(${TEST_APP} PRIVATE daq::coreobjects
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
