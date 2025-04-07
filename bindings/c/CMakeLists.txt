set(LIB_NAME copendaq)
set_cmake_folder_context(TARGET_FOLDER_NAME copendaq)

set(SRC_CoreTypes_Headers
    ${CMAKE_CURRENT_LIST_DIR}/include/copendaq.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccommon.h

    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/factories.h

    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/base_object.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/binarydata.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/boolean.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/complex_number.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/convertible.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/dictobject.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/enumeration.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/enumeration_type.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/event.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/event_args.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/event_handler.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/float.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/freezable.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/function.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/integer.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/iterable.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/iterator.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/listobject.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/number.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/procedure.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/ratio.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/serializable.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/serialized_list.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/serialized_object.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/serializer.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/simple_type.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/stringobject.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/struct_builder.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/struct.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/struct_type.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/type.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/type_manager.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/updatable.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoretypes/version_info.h
)

set(SRC_Private_Headers
    ${CMAKE_CURRENT_LIST_DIR}/include/copendaq_private.h
    ${CMAKE_CURRENT_LIST_DIR}/include/private/utils.h
    ${CMAKE_CURRENT_LIST_DIR}/include/private/c_event_handler_impl.h
)

set(SRC_CoreTypes_Cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/factories.cpp

    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/base_object.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/binarydata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/boolean.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/complex_number.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/convertible.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/dictobject.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/enumeration.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/enumeration_type.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/event.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/event_args.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/event_handler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/float.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/freezable.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/function.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/integer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/iterable.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/iterator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/listobject.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/number.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/procedure.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/ratio.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/serializable.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/serialized_list.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/serialized_object.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/serializer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/simple_type.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/stringobject.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/struct_builder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/struct.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/struct_type.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/type.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/type_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/updatable.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoretypes/version_info.cpp
)

set(SRC_Private_Cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/private/utils.cpp
)

set(SRC_CoreObjects_Headers
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/common.h

    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/argument_info.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/authentication_provider.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/callable_info.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/coercer.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/core_event_args.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/end_update_event_args.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/eval_value.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/ownable.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/permission_manager.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/permission_mask_builder.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/permissions.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/permissions_builder.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/property.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/property_builder.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/property_object.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/property_object_class.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/property_object_class_builder.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/property_object_protected.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/property_value_event_args.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/unit.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/unit_builder.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/user.h
    ${CMAKE_CURRENT_LIST_DIR}/include/ccoreobjects/validator.h
)

set(SRC_CoreObjects_Cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/argument_info.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/authentication_provider.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/callable_info.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/coercer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/core_event_args.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/end_update_event_args.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/eval_value.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/ownable.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/permission_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/permission_mask_builder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/permissions.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/permissions_builder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/property.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/property_builder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/property_object.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/property_object_class.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/property_object_class_builder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/property_object_protected.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/property_value_event_args.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/unit.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/unit_builder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/user.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ccoreobjects/validator.cpp
)

add_library(${LIB_NAME} SHARED ${SRC_CoreTypes_Headers} ${SRC_CoreTypes_Cpp} ${SRC_Private_Headers} ${SRC_Private_Cpp} ${SRC_CoreObjects_Headers} ${SRC_CoreObjects_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PRIVATE
    daq::coretypes
    daq::coreobjects
    daq::opendaq
)

target_include_directories(${LIB_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

target_compile_definitions(${LIB_NAME} PRIVATE BUILDING_SHARED_LIBRARY)

set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${SRC_CoreTypes_Headers} ${SRC_CoreObjects_Headers}")

install(TARGETS ${LIB_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT ${SDK_NAME}_${BASE_NAME}_Runtime
    NAMELINK_COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
    PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${BASE_NAME}/copendaq
    COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
)

if(OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
