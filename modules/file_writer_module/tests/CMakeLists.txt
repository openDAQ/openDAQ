set(MODULE_NAME file_writer_module)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_file_writer_module.cpp
                 test_app.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          PRIVATE Arrow::arrow_shared parquet_shared
)

target_include_directories(${TEST_APP} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<BUILD_INTERFACE:${arrow_SOURCE_DIR}/cpp/src>
                                              $<INSTALL_INTERFACE:include>
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
