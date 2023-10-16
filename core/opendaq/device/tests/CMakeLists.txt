set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME device)

set(TEST_HEADERS
)

set(TEST_SOURCES test_factories.cpp
                 test_device_info.cpp
                 test_device.cpp
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

target_link_libraries(${TEST_APP} PRIVATE daq::coreobjects
                                          daq::opendaq
                                          daq::opendaq_mocks
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
