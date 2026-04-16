set(MODULE_NAME signal_generator)
set(TEST_APP test_${MODULE_NAME})

add_executable(${TEST_APP}
    test_signal_generator.cpp
    test_app.cpp
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /bigobj)
endif()

target_link_libraries(${TEST_APP} PRIVATE
    ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
    ${SDK_TARGET_NAMESPACE}::opendaq
    ${SDK_TARGET_NAMESPACE}::opendaq_mocks
    ${SDK_TARGET_NAMESPACE}::test_utils
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)
target_include_directories(${TEST_APP} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_test(NAME ${TEST_APP}
    COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if(OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
