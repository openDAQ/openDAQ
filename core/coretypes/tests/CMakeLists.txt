set(MODULE_NAME coretypes)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_objectptr.cpp
                 test_baseobject.cpp
                 test_stringobject.cpp
                 test_float.cpp
                 test_complex_number.cpp
                 test_integer.cpp
                 test_boolean.cpp
                 test_dictobject.cpp
                 test_listobject.cpp
                 test_function.cpp
                 test_procedure.cpp
                 test_callback.cpp
                 test_function_custom.cpp
                 test_procedure_custom.cpp
                 test_binary_data.cpp
                 integration_test.cpp
                 cycleref_test.cpp
                 test_version.cpp
                 test_objcount.cpp
                 test_objectptr_op.cpp
                 test_weakref.cpp
                 test_json_deserializer.cpp
                 test_json_serializer.cpp
                 test_json_serialized_list.cpp
                 test_json_serialized_object.cpp
                 test_errorinfo.cpp
                 test_ratio.cpp
                 test_event_args.cpp
                 test_event.cpp
                 test_intfid.cpp
                 test_type_name.cpp
                 test_type_traits.cpp
                 test_type_name_complex.cpp
                 test_type_list.cpp
                 test_actual_interfaces.cpp
                 test_simplified_ratio.cpp
                 test_iterator.cpp
                 test_iterable.cpp
                 test_cycle_detector.cpp
                 test_internaladdref.cpp
                 test_struct.cpp
                 test_type.cpp
                 test_type_manager.cpp
                 test_callback_factory_deserialization.cpp
)

set(TEST_HEADERS event_test.h
)

add_executable(${TEST_APP} test_app.cpp
                           ${TEST_SOURCES}
                           ${TEST_HEADERS}

)

target_compile_definitions(${TEST_APP} PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})
set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${TEST_APP}
        PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                ${SDK_TARGET_NAMESPACE}::test_utils
)

add_test(NAME ${TEST_APP}
         COMMAND $<TARGET_FILE_NAME:${TEST_APP}>
         WORKING_DIRECTORY bin
)

if(OPENDAQ_ENABLE_COVERAGE)
    setup_target_for_coverage(${MODULE_NAME}coverage ${TEST_APP} ${MODULE_NAME}coverage)
endif()

if (TEST_BINDINGS)
    add_subdirectory(bindings)
endif()
