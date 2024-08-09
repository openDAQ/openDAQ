set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME synchronization)

set(TEST_HEADERS
)

set(TEST_SOURCES
    test_sync_component.cpp
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

target_link_libraries(${TEST_APP} PRIVATE daq::coreobjects
                                          daq::opendaq
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
