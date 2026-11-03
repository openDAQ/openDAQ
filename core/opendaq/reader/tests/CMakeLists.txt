opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME reader)

set(TEST_HEADERS reader_common.h
                 multi_reader2/multi_reader2_harness.h
                 multi_reader2/multi_reader2_harness_detail.h
                 multi_reader2/multi_reader2_harness_state.h
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
                 test_stream_reader_domain_switch.cpp
                 multi_reader2/multi_reader2_harness.cpp
                 multi_reader2/multi_reader2_harness_owner.cpp
                 multi_reader2/multi_reader2_harness_concurrency.cpp
                 multi_reader2/test_harness_self_checks.cpp
                 multi_reader2/test_read_calls_and_counts.cpp
                 multi_reader2/test_value_types_and_domains.cpp
                 multi_reader2/test_synchronization.cpp
                 multi_reader2/test_configure_and_construction.cpp
                 multi_reader2/test_error_policy_and_main_input.cpp
                 multi_reader2/test_recovery.cpp
                 multi_reader2/test_limits.cpp
                 multi_reader2/test_events.cpp
                 multi_reader2/test_handler_and_lifetime.cpp
                 multi_reader2/test_scenarios.cpp
                 multi_reader2/test_concurrency.cpp
                 multi_reader2/test_ticks_conversion_and_queue.cpp
                 multi_reader2/test_ingest_task.cpp
                 multi_reader2/test_regressions_data_manager.cpp
                 multi_reader2/test_regressions_api.cpp
                 ../src/multi_reader2_domain.cpp
                 ../src/multi_reader2_convert.cpp
                 ../src/multi_reader2_queue.cpp
                 ../src/multi_reader2_ingest_task.cpp
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
                                          $<BUILD_INTERFACE:spdlog>  # the MultiReader2 harness's error-counting logger sink
)

opendaq_target_pch_group(${TEST_APP} opendaq_tests <opendaq/opendaq.h> <gtest/gtest.h>)
opendaq_target_unity_tests(${TEST_APP} BATCH_SIZE 4)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)
# MultiReader2 Suite 3 draws its interleavings from a seed; a fixed one keeps CI runs comparable. Soak runs leave it unset
set_tests_properties(${TEST_APP} PROPERTIES ENVIRONMENT MR2_SEED=1)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
