set(MODULE_NAME opcuatms_client)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_client_context.cpp
)

add_executable(${TEST_APP} test_app.cpp
                           ${TEST_SOURCES}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

if (WIN32)
    set(BCRYPT_LIB bcrypt.dll)
endif()

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::test_utils
                                          ${BCRYPT_LIB}
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
