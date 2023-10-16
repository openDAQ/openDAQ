set(MODULE_NAME coreobjects)
set(TEST_APP test_${MODULE_NAME})

set(TEST_SOURCES test_property.cpp
                 test_version.cpp
                 test_property_object.cpp
                 test_property_object_class.cpp
                 test_property_object_manager.cpp
                 customobject_test.cpp
                 test_evalvalue.cpp
                 amplifier_test.cpp
                 serialization_test.cpp
                 test_derivedpropertyobject.cpp
                 test_owning_list.cpp
                 test_owning_dict.cpp
                 test_object.cpp
                 test_object_property_class.cpp
                 test_exceptions.cpp
                 test_property_object_update.cpp
                 test_multi_ptr.cpp
                 test_callable_info.cpp
                 test_argument_info.cpp
                 test_value_changed_events.cpp
				 test_unit.cpp
)

set(TEST_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/coreobjects_test.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_common.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_object.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_object_ptr.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_object_impl.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_object_factory.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_object_property_class.h

                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_value_object.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_value_object_ptr.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_value_object_impl.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/include/test_value_object_factory.h
)

add_executable(${TEST_APP} test_app.cpp
                           ${TEST_HEADERS}
                           ${TEST_SOURCES}
)

set_target_properties(${TEST_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_compile_definitions(${TEST_APP} PRIVATE ${OPENDAQ_TEST_COMPILE_DEFINES})

target_link_libraries(${TEST_APP}
        PRIVATE ${SDK_TARGET_NAMESPACE}::${MODULE_NAME}
                ${SDK_TARGET_NAMESPACE}::test_utils
)

target_include_directories(${TEST_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
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
