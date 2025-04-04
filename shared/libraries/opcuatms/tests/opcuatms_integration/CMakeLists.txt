set(MODULE_NAME opcuatms_integration)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES main.cpp
                 tms_object_integration_test.h
                 tms_object_integration_test.cpp
                 test_tms_signal.cpp
                 test_tms_function_block.cpp
                 test_tms_channel.cpp
                 test_tms_input_port.cpp
                 test_tms_property.cpp
                 test_tms_property_object.cpp
                 test_tms_device.cpp
                 test_tms_integration.cpp
                 test_property_object_advanced.cpp
                 test_tms_amplifier.cpp
                 test_tms_component.cpp
                 test_tms_folder.cpp
                 test_tms_function_property.cpp
                 test_tms_fusion_device.cpp
                 test_tms_function_block_type.cpp
)

if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    list(APPEND TEST_SOURCES test_streaming_integration.cpp)
endif()

set(SUPPORTS_ASAN 0)

if(CXX_COMPILER_ID EQUAL Clang OR CXX_COMPILER_ID EQUAL AppleClang OR CXX_COMPILER_ID EQUAL GNU)
    set(SUPPORTS_ASAN 1)
    set(ASAN_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
endif()

add_executable(${TEST_APP} ${TEST_SOURCES})

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

if (WIN32)
    set(BCRYPT_LIB bcrypt.dll)
endif()

set(StreamingLibraries )

if (OPENDAQ_ENABLE_WEBSOCKET_STREAMING)
    list(APPEND StreamingLibraries PRIVATE daq::websocket_streaming)
endif()

target_link_libraries(${TEST_APP} PRIVATE daq::opcuatms_test_utils
                                          daq::opcuatms_server
                                          daq::opcuatms_client
                                          daq::opendaq_mocks
                                          ${BCRYPT_LIB}
                                          Taskflow::Taskflow
                                          ${StreamingLibraries}
)

if(SUPPORTS_ASAN)
    target_link_libraries(${TEST_APP} PRIVATE asan)
    target_compile_options(${TEST_APP} PRIVATE -Wall -Werror ${CGOV_COMPILE_FLAGS} ${ASAN_COMPILE_FLAGS})
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (MSVC)
    target_compile_options(${TEST_APP} PRIVATE /wd4324  /bigobj)
elseif (MINGW AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${TEST_APP} PRIVATE -Wa,-mbig-obj)
endif()

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}testcoverage ${TEST_APP} ${MODULE_NAME}testcoverage)
endif()
