set_cmake_folder_context(TARGET_FOLDER_NAME)
set(MODULE_NAME ${SDK_TARGET_NAME})

set(TEST_HEADERS test_helpers.h
                 test_config_provider.h
)

set(TEST_SOURCES test_factories.cpp
                 test_instance.cpp
                 test_core_events.cpp
                 test_config_provider.cpp
                 test_access_control.cpp
                 ${TEST_HEADERS}
)

opendaq_prepare_test_runner(TEST_APP FOR ${MODULE_NAME}
                       SOURCES
                           test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

target_link_libraries(${TEST_APP} PRIVATE daq::opendaq_mocks
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE:${TEST_APP}>
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
