set(MODULE_NAME parquet_recorder_module)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_parquet_recorder_module.cpp test_app.cpp)

add_executable(${TEST_APP} ${TEST_SOURCES})

target_link_libraries(
  ${TEST_APP} PRIVATE daq::test_utils testlibs ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                      Arrow::arrow_static parquet_static)

add_test(
  NAME ${TEST_APP}
  COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
  WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if(MSVC)
  set_property(
    TARGET ${TEST_APP}
    APPEND_STRING
    PROPERTY COMPILE_FLAGS "/wd4324")
endif()

if(OPENDAQ_ENABLE_COVERAGE)
  setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
