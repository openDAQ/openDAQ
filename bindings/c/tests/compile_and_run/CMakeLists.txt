set(MODULE_NAME c_compile_and_run)
set(TEST_APP test_${MODULE_NAME})

enable_language(C)

set(TEST_SOURCES
        test_compile.c
)

add_executable(${TEST_APP} ${TEST_SOURCES})

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP} PRIVATE daq::copendaq)

add_test(NAME ${TEST_APP}
        COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)
