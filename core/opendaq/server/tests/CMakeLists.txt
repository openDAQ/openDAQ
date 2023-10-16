set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME server)

set(TEST_HEADERS
)

set(TEST_MOCKS mock/mock_server.h
               mock/mock_server.cpp
)

set(TEST_SOURCES test_server_type.cpp
                 test_server.cpp
                 ${TEST_HEADERS}
                 ${TEST_MOCKS}
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

target_link_libraries(${TEST_APP}
    PRIVATE
        daq::coreobjects
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

source_group("Mocks" FILES ${TEST_MOCKS})

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
