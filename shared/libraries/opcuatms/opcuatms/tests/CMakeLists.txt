set(MODULE_NAME opcuatms)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_core_types_utils.cpp
                 test_exception.cpp
                 test_variant_converter.cpp
                 test_variant_list_converter.cpp
                 test_extension_object.cpp
                 test_generic_struct_converter.cpp
)

add_executable(${TEST_APP} testapp.cpp
                           ${TEST_SOURCES}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                                          daq::test_utils
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if (OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()
