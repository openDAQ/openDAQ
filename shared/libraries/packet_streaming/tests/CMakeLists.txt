set(BASE_NAME packet_streaming)
set(MODULE_NAME ${SDK_TARGET_NAME}_${BASE_NAME})
set(TEST_APP test_${MODULE_NAME})

add_executable(${TEST_APP}
    test_packet_streaming.cpp
    packet_transmission.h
    packet_transmission.cpp
)

target_link_libraries(${TEST_APP} PRIVATE
    ${SDK_TARGET_NAMESPACE}::${BASE_NAME}
    daq::opendaq
    GTest::GTest GTest::Main
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

add_test(NAME ${TEST_APP}
    COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
    WORKING_DIRECTORY bin
)

if(OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
