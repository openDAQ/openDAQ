# Shared resources that test binaries occupy, so that `ctest -j` never runs two of their users at once.
#
#   network_native  TCP ports of the native streaming / configuration servers (7420-7422)
#   network_lt      TCP ports of the LT streaming servers (7414, 7415)
#   network_opcua   TCP ports of the OPC UA servers (4840-4843)
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

# opendaq_add_test_shards(<target> <count>)
# Registers <count> ctest entries that each run a disjoint gtest shard of <target>. Every shard gets its own
# OPENDAQ_TEST_PORT_OFFSET (read by the device module test helpers), so the shards run concurrently with each
# other and with the tests on the default ports, and none of them needs a RESOURCE_LOCK.
# When GTEST_OUTPUT names a directory (the CI does), each shard writes its own <target>_<index>.xml there; without it
# gtest only warns about the appended file name.
function(opendaq_add_test_shards TARGET COUNT)
    math(EXPR last "${COUNT} - 1")
    foreach(index RANGE ${last})
        set(name ${TARGET}_${index})
        math(EXPR offset "(${index} + 1) * 1000")  # 1000 apart: +100 lands on ports Windows services listen on (5040)
        add_test(NAME ${name}
                 COMMAND $<TARGET_FILE_NAME:${TARGET}>
                 WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
        )
        # PROCESSORS 2: a shard runs a server and a client instance, so ctest schedules fewer of them at once
        set_tests_properties(${name} PROPERTIES
            ENVIRONMENT "GTEST_TOTAL_SHARDS=${COUNT};GTEST_SHARD_INDEX=${index};OPENDAQ_TEST_PORT_OFFSET=${offset}"
            ENVIRONMENT_MODIFICATION "GTEST_OUTPUT=string_append:${name}.xml"
            PROCESSORS 2
        )
    endforeach()
endfunction()
