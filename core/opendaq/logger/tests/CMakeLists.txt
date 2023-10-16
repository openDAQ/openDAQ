set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME logger)

set(TEST_HEADERS invalid_logger_sink.h
                 should_log.h

)
set(TEST_SOURCES test_logger.cpp
                 test_logger_component.cpp
                 test_logger_sink.cpp
)

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

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
