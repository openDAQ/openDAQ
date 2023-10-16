set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME streaming)

set(TEST_MOCKS mock/mock_signal_remote.h
               mock/mock_signal_remote.cpp
)

set(TEST_SOURCES test_streaming.cpp
                 test_signal_remote.cpp
                 test_streaming_info.cpp
                 ${TEST_MOCKS}
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP}
    PRIVATE
        daq::coreobjects
        daq::opendaq_mocks
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
