set(MODULE_NAME opcuashared)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES
    main.cpp
    test_string_utils.cpp
    test_status_code.cpp
    test_opcuaendpoint.cpp
    test_opcuanodeid.cpp
    test_opcua_variant.cpp
    test_opcua_object.cpp
    test_opcuadatavalue.cpp
    test_opcuacallmethodresult.cpp
    test_opcuaversion.cpp
    test_opcua_vector.cpp
    test_opcuadatatypearraylist.cpp
    test_bcrypt.cpp
)

add_executable(${TEST_APP} ${TEST_SOURCES})

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::test_utils
)

if (MSVC)
    target_compile_definitions(${TEST_APP} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
