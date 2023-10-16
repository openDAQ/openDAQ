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
    generated/py_validator.cpp
    generated/py_unit.cpp
    generated/py_unit_builder.cpp
    generated/py_component_type.cpp
)

pybind11_add_module(${LIB_NAME}
    STATIC STRIP_UNNEEDED
        ${SRC_Headers}
        ${SRC_Cpp}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::py_core_types
        daq::coreobjects
)

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)
