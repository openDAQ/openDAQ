set(MODULE_NAME ${SDK_TARGET_NAME})
set(TEST_APP test_${MODULE_NAME})

set(TEST_HEADERS test_helpers.h
)

set(TEST_SOURCES test_factories.cpp
                 test_instance.cpp
                 test_core_events.cpp
                 ${TEST_HEADERS}
)

add_executable(${TEST_APP} test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::test_utils
                                          daq::opendaq_mocks
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE:${TEST_APP}>
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(${TEST_APP} PRIVATE OPENDAQ_SKIP_DLL_IMPORT)

source_group("Mocks" FILES ${TEST_MOCKS})

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
