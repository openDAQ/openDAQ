set(MODULE_NAME py_opendaq_module)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_python_module.cpp
                 test_python_function_block.cpp
)

add_executable(${TEST_APP} test_app.cpp
                           ${TEST_SOURCES}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::test_utils
)

target_compile_definitions(${TEST_APP} PRIVATE MOCK_MODULE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/mock")

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
