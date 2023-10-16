set(MODULE_NAME opcuaclient)
set(TEST_APP test_${MODULE_NAME})

set(SRC_Cpp main.cpp
            opcuaservertesthelper.cpp
            test_opcuanodefactory.cpp
            test_opcuanodefactorybrowser.cpp
            test_opcuaasyncexecthread.cpp
            test_opcuataskprocessor.cpp
            test_opcuatimertaskhelper.cpp
            test_opcuaclient.cpp
)

set(SRC_Include opcuaservertesthelper.h
)

opendaq_prepend_path("include" SRC_Include)
opendaq_prepend_path("src" SRC_Cpp)

add_executable(${TEST_APP} ${SRC_Cpp}
                           ${SRC_Include}
)

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::test_utils
                                          daq::opcuaserver #remove after moving test_opcuaclientbasedsacqcontrol to opcua server/client integration tests
)

target_include_directories(${TEST_APP} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if(OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
