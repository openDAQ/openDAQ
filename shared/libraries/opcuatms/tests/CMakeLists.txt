cmake_minimum_required(VERSION 3.10)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(opcuatms_integration_tests CXX)

add_subdirectory(test_utils)

option(OPENDAQ_ENABLE_OPCUA_INTEGRATION_TESTS "Enable OpcUa integration testing" OFF)

if (OPENDAQ_ENABLE_OPCUA_INTEGRATION_TESTS)
    add_subdirectory(opcuatms_integration)
endif()
