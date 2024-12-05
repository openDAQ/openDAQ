set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME device)

set(TEST_HEADERS
)

set(TEST_SOURCES test_factories.cpp
                 test_device_info.cpp
                 test_device.cpp
                 test_tree_traversal.cpp
                 test_server_capability.cpp
                 test_device_domain.cpp
                 test_log_file_info.cpp
                 test_user_lock.cpp
                 test_connection_status_container.cpp
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

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
