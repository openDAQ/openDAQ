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

# Separate binary from ${TEST_APP}: it needs to call Py_Initialize() itself, before anything
# else touches PythonRuntime::instance(), to exercise PythonRuntime's "attached to an
# already-running interpreter" branch - see test_python_runtime_attach.cpp for why that can't
# share a process with the other tests here.
set(ATTACH_TEST_APP test_${MODULE_NAME}_attach)

add_executable(${ATTACH_TEST_APP} test_python_runtime_attach.cpp)

set_target_properties(${ATTACH_TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${ATTACH_TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                                  daq::test_utils
)

target_compile_definitions(${ATTACH_TEST_APP} PRIVATE MOCK_MODULE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/mock")

add_test(NAME ${ATTACH_TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${ATTACH_TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${ATTACH_TEST_APP}>
)
