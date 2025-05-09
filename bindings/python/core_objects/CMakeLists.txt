set(LIB_NAME py_core_objects)

set(SRC_Headers
    include/py_core_objects/py_core_objects.h
)
set(SRC_Cpp
    py_core_objects.cpp
    generated/py_argument_info.cpp
    generated/py_callable_info.cpp
    generated/py_coercer.cpp
    generated/py_eval_value.cpp
    generated/py_ownable.cpp
    generated/py_property.cpp
    generated/py_property_builder.cpp
    generated/py_property_object.cpp
    generated/py_property_object_class.cpp
    generated/py_property_object_class_builder.cpp
    generated/py_property_object_protected.cpp
    generated/py_property_value_event_args.cpp
    generated/py_core_event_args.cpp
    generated/py_validator.cpp
    generated/py_unit.cpp
    generated/py_unit_builder.cpp
    generated/py_user.cpp
    generated/py_authentication_provider.cpp
    generated/py_permissions_builder.cpp
    generated/py_permission_mask_builder.cpp
    generated/py_permission_manager.cpp
    generated/py_permissions.cpp
)

add_library(${LIB_NAME} STATIC ${SRC_Headers} ${SRC_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::py_core_types
        daq::coreobjects
    PRIVATE
        pybind11::headers
        pybind11::module
        pybind11::lto
)

if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
    set_target_properties(${LIB_NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
endif()

if(MSVC)
    target_link_libraries(${LIB_NAME} 
        PRIVATE 
            pybind11::windows_extras
    )
endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)
