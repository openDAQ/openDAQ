set(MODULE_NAME ccoretypes)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES
        test_ccoretypes.cpp
)

set(TEST_HEADERS
)

add_executable(${TEST_APP} ${CMAKE_CURRENT_LIST_DIR}/../test_app.cpp
        ${TEST_SOURCES}
        ${TEST_HEADERS}

)

target_compile_definitions(${TEST_APP} PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP}
        PRIVATE daq::copendaq
        daq::test_utils testlibs
)

add_test(NAME ${TEST_APP}
        COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)
