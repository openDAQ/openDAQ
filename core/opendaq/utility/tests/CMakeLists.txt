set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME utility)

##############################
# DLL Test

set(TEST_HEADERS
)

set(TEST_SOURCES
    test_app.cpp
    test_ids_parser.cpp
    test_mem_pool_allocator.cpp
    test_packet_buffer.cpp
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

##############################
# CTest setup

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
