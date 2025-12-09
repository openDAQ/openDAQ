set(MODULE_NAME opcuaserver)
set(TEST_APP test_${MODULE_NAME})

set(SRC_Cpp main.cpp
            common_test_functions.h
            test_opcuaserver.cpp
            test_opcuaservernode.cpp
            test_opcuaservernodefactory.cpp
            test_opcuaserverlock.cpp
            test_opcuasession.cpp
)

add_executable(${TEST_APP} ${SRC_Cpp})

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::opendaq_utils
                                          daq::test_utils
                                          daq::opcuaclient
                                          daq::opcua_daq_types
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_APP}>
)

if(OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
