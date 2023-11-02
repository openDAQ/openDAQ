set(TEST_APP test_siggen_integration)

set(TEST_SOURCES test_websocket_siggen.cpp
                 test_app.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES}
)

target_link_libraries(${TEST_APP} PRIVATE daq::test_utils
                                          daq::opendaq
)

add_dependencies(${TEST_APP} daq::ws_stream_cl_module
                             siggen2websocket
)

find_program(BASH_PROGRAM bash)
if(BASH_PROGRAM)
    message(STATUS "Bash found - add siggen test")
    set(TEST_SCRIPT "\
        $<TARGET_FILE:siggen2websocket> 7413 ${CMAKE_CURRENT_SOURCE_DIR}/siggen_config.json 1>/tmp/siggen_output 2>/tmp/siggen_output 0</dev/null & \
        $<TARGET_FILE:${TEST_APP}>; \
        EXIT_CODE=$?; \
        killall siggen2websocket; \
        echo \"\nsiggen2websocket output:\n\n$(</tmp/siggen_output)\"; \
        exit $EXIT_CODE"
    )
    add_test(NAME ${TEST_APP}
             COMMAND ${BASH_PROGRAM} -c "${TEST_SCRIPT}"
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

target_compile_definitions(${TEST_APP} PRIVATE OPENDAQ_SKIP_DLL_IMPORT)

set_target_properties(${TEST_APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${TEST_APP}coverage ${TEST_APP} ${TEST_APP}coverage)
endif()
