set(BASE_NAME bcrypt)
set(TEST_APP test_${BASE_NAME})

add_executable(${TEST_APP}
    test_app.cpp
    test_bcrypt.cpp
)

target_link_libraries(${TEST_APP} PRIVATE
    ${BASE_NAME}
    daq::test_utils
    GTest::GTest GTest::Main
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

add_test(NAME ${TEST_APP}
    COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
    WORKING_DIRECTORY bin
)

if(OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${BASE_NAME}coverage ${TEST_APP} ${BASE_NAME}coverage)
endif()
