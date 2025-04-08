set(LIB_NAME copendaq)
set_cmake_folder_context(TARGET_FOLDER_NAME copendaq)

set(SRC_CoreTypes_Headers
    include/copendaq.h
    include/ccommon.h

    include/ccoretypes/factories.h

    include/ccoretypes/base_object.h
    include/ccoretypes/binarydata.h
    include/ccoretypes/boolean.h
    include/ccoretypes/complex_number.h
    include/ccoretypes/convertible.h
    include/ccoretypes/dictobject.h
    include/ccoretypes/enumeration.h
    include/ccoretypes/enumeration_type.h
    include/ccoretypes/event.h
    include/ccoretypes/event_args.h
    include/ccoretypes/event_handler.h
    include/ccoretypes/float.h
    include/ccoretypes/freezable.h
    include/ccoretypes/function.h
    include/ccoretypes/integer.h
    include/ccoretypes/iterable.h
    include/ccoretypes/iterator.h
    include/ccoretypes/listobject.h
    include/ccoretypes/number.h
    include/ccoretypes/procedure.h
    include/ccoretypes/ratio.h
    include/ccoretypes/serializable.h
    include/ccoretypes/serialized_list.h
    include/ccoretypes/serialized_object.h
    include/ccoretypes/serializer.h
    include/ccoretypes/simple_type.h
    include/ccoretypes/stringobject.h
    include/ccoretypes/struct_builder.h
    include/ccoretypes/struct.h
    include/ccoretypes/struct_type.h
    include/ccoretypes/type.h
    include/ccoretypes/type_manager.h
    include/ccoretypes/updatable.h
    include/ccoretypes/version_info.h
)

set(SRC_Private_Headers
    include/copendaq_private.h
    include/private/utils.h
    include/private/c_event_handler_impl.h
)

set(SRC_CoreTypes_Cpp
    src/ccoretypes/factories.cpp

    src/ccoretypes/base_object.cpp
    src/ccoretypes/binarydata.cpp
    src/ccoretypes/boolean.cpp
    src/ccoretypes/complex_number.cpp
    src/ccoretypes/convertible.cpp
    src/ccoretypes/dictobject.cpp
    src/ccoretypes/enumeration.cpp
    src/ccoretypes/enumeration_type.cpp
    src/ccoretypes/event.cpp
    src/ccoretypes/event_args.cpp
    src/ccoretypes/event_handler.cpp
    src/ccoretypes/float.cpp
    src/ccoretypes/freezable.cpp
    src/ccoretypes/function.cpp
    src/ccoretypes/integer.cpp
    src/ccoretypes/iterable.cpp
    src/ccoretypes/iterator.cpp
    src/ccoretypes/listobject.cpp
    src/ccoretypes/number.cpp
    src/ccoretypes/procedure.cpp
    src/ccoretypes/ratio.cpp
    src/ccoretypes/serializable.cpp
    src/ccoretypes/serialized_list.cpp
    src/ccoretypes/serialized_object.cpp
    src/ccoretypes/serializer.cpp
    src/ccoretypes/simple_type.cpp
    src/ccoretypes/stringobject.cpp
    src/ccoretypes/struct_builder.cpp
    src/ccoretypes/struct.cpp
    src/ccoretypes/struct_type.cpp
    src/ccoretypes/type.cpp
    src/ccoretypes/type_manager.cpp
    src/ccoretypes/updatable.cpp
    src/ccoretypes/version_info.cpp
)

set(SRC_Private_Cpp
    src/private/utils.cpp
)

set(SRC_CoreObjects_Headers
    include/ccoreobjects/common.h

    include/ccoreobjects/argument_info.h
    include/ccoreobjects/authentication_provider.h
    include/ccoreobjects/callable_info.h
    include/ccoreobjects/coercer.h
    include/ccoreobjects/core_event_args.h
    include/ccoreobjects/end_update_event_args.h
    include/ccoreobjects/eval_value.h
    include/ccoreobjects/ownable.h
    include/ccoreobjects/permission_manager.h
    include/ccoreobjects/permission_mask_builder.h
    include/ccoreobjects/permissions.h
    include/ccoreobjects/permissions_builder.h
    include/ccoreobjects/property.h
    include/ccoreobjects/property_builder.h
    include/ccoreobjects/property_object.h
    include/ccoreobjects/property_object_class.h
    include/ccoreobjects/property_object_class_builder.h
    include/ccoreobjects/property_object_protected.h
    include/ccoreobjects/property_value_event_args.h
    include/ccoreobjects/unit.h
    include/ccoreobjects/unit_builder.h
    include/ccoreobjects/user.h
    include/ccoreobjects/validator.h
)

set(SRC_CoreObjects_Cpp
    src/ccoreobjects/argument_info.cpp
    src/ccoreobjects/authentication_provider.cpp
    src/ccoreobjects/callable_info.cpp
    src/ccoreobjects/coercer.cpp
    src/ccoreobjects/core_event_args.cpp
    src/ccoreobjects/end_update_event_args.cpp
    src/ccoreobjects/eval_value.cpp
    src/ccoreobjects/ownable.cpp
    src/ccoreobjects/permission_manager.cpp
    src/ccoreobjects/permission_mask_builder.cpp
    src/ccoreobjects/permissions.cpp
    src/ccoreobjects/permissions_builder.cpp
    src/ccoreobjects/property.cpp
    src/ccoreobjects/property_builder.cpp
    src/ccoreobjects/property_object.cpp
    src/ccoreobjects/property_object_class.cpp
    src/ccoreobjects/property_object_class_builder.cpp
    src/ccoreobjects/property_object_protected.cpp
    src/ccoreobjects/property_value_event_args.cpp
    src/ccoreobjects/unit.cpp
    src/ccoreobjects/unit_builder.cpp
    src/ccoreobjects/user.cpp
    src/ccoreobjects/validator.cpp
)

add_library(${LIB_NAME} SHARED ${SRC_CoreTypes_Headers} ${SRC_CoreTypes_Cpp} ${SRC_Private_Headers} ${SRC_Private_Cpp} ${SRC_CoreObjects_Headers} ${SRC_CoreObjects_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_sources(${LIB_NAME}
    INTERFACE
    FILE_SET copendaq_interface_headers TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES ${SRC_CoreTypes_Headers} ${SRC_CoreObjects_Headers}
)

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

install(TARGETS ${LIB_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT ${SDK_NAME}_${BASE_NAME}_Runtime
    NAMELINK_COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
    FILE_SET copendaq_interface_headers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${BASE_NAME}/copendaq
    COMPONENT ${SDK_NAME}_${BASE_NAME}_Development
)

if(OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
