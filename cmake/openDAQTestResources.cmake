# Shared resources that test binaries occupy, so that `ctest -j` never runs two of their users at once.
#
#   network_native  TCP ports of the native streaming / configuration servers (7420-7422)
#   network_lt      TCP ports of the LT streaming servers (7414, 7415)
#   network_opcua   TCP ports of the OPC UA servers (4840-4843)
#   network_opcua_tms  port 4860 of the OPC UA library test servers (TMS_TEST_OPCUA_PORT in OpcUaModules)
#   mdns            advertises servers over mDNS or asserts on discovery results
#
# Tests without any of these run concurrently with everything else.

# opendaq_test_resource_lock(<test> <resource>... [DIRECTORY <dir>])
# Sets RESOURCE_LOCK on a test; DIRECTORY names the source directory of a test defined by another project.
function(opendaq_test_resource_lock TEST_NAME)
    cmake_parse_arguments(ARG "" "DIRECTORY" "" ${ARGN})

    if(NOT ARG_DIRECTORY)
        set_tests_properties(${TEST_NAME} PROPERTIES RESOURCE_LOCK "${ARG_UNPARSED_ARGUMENTS}")
        return()
    endif()

    # if(TEST) only sees the current directory; the test targets share the test names
    if(NOT TARGET ${TEST_NAME})
        return()
    endif()

    if(CMAKE_VERSION VERSION_LESS 3.28)
        message(WARNING "CMake ${CMAKE_VERSION} cannot set RESOURCE_LOCK on ${TEST_NAME} (defined in ${ARG_DIRECTORY}); "
                        "run ctest without -j or upgrade to CMake 3.28")
        return()
    endif()

    set_property(TEST ${TEST_NAME} DIRECTORY ${ARG_DIRECTORY} PROPERTY RESOURCE_LOCK ${ARG_UNPARSED_ARGUMENTS})
endfunction()

# Locks for the tests of the module projects fetched by external_modules/CMakeLists.txt.
function(opendaq_lock_external_module_tests)
    if(OPCUA_MODULES_SOURCE_DIR)
        set(OPCUA ${OPCUA_MODULES_SOURCE_DIR})
        opendaq_test_resource_lock(test_opcua               network_opcua DIRECTORY ${OPCUA}/shared/libraries/opcua/tests/testopcua)
        opendaq_test_resource_lock(test_opcuaclient         network_opcua DIRECTORY ${OPCUA}/shared/libraries/opcua/opcuaclient/tests)
        opendaq_test_resource_lock(test_opcuaserver         network_opcua DIRECTORY ${OPCUA}/shared/libraries/opcua/opcuaserver/tests)
        opendaq_test_resource_lock(test_opcuashared         network_opcua DIRECTORY ${OPCUA}/shared/libraries/opcua/opcuashared/tests)
        opendaq_test_resource_lock(test_opcuatms_client     network_opcua DIRECTORY ${OPCUA}/shared/libraries/opcuatms/opcuatms_client/tests)
        opendaq_test_resource_lock(test_opcuatms_server     network_opcua_tms DIRECTORY ${OPCUA}/shared/libraries/opcuatms/opcuatms_server/tests)
        opendaq_test_resource_lock(test_opcuatms_integration network_opcua_tms DIRECTORY ${OPCUA}/shared/libraries/opcuatms/tests/opcuatms_integration)
        opendaq_test_resource_lock(test_opcua_client_module network_opcua mdns DIRECTORY ${OPCUA}/modules/opcua_client_module/tests)
        opendaq_test_resource_lock(test_opcua_server_module network_opcua mdns DIRECTORY ${OPCUA}/modules/opcua_server_module/tests)
    endif()

    if(LT_MODULES_SOURCE_DIR)
        set(LT ${LT_MODULES_SOURCE_DIR})
        opendaq_test_resource_lock(test_ws_stream_cl_module network_lt DIRECTORY ${LT}/modules/websocket_streaming_client_module/tests)
        opendaq_test_resource_lock(test_ws_stream_srv_module network_lt DIRECTORY ${LT}/modules/websocket_streaming_server_module/tests)
    endif()
endfunction()
