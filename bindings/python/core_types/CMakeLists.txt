set(LIB_NAME py_core_types)

set(SRC_Headers
    include/py_core_types/py_core_types.h
    #
    include/py_core_types/py_base_object.h
    include/py_core_types/py_converter.h
    include/py_core_types/py_opendaq_daq.h
    include/py_core_types/py_list_object.h
    include/py_core_types/py_procedure.h
    include/py_core_types/py_function.h
)

set(SRC_Cpp
    src/py_core_types.cpp
    #
    src/py_opendaq_daq.cpp
    src/py_converter.cpp
    #
    src/py_base_object.cpp
    src/py_integer.cpp
    src/py_float.cpp
    src/py_boolean.cpp
    src/py_string.cpp
    src/py_ratio.cpp
    src/py_complex_number.cpp
    src/py_number.cpp
    #
    src/py_iterable.cpp
    src/py_iterator.cpp
    src/py_list.cpp
    src/py_dict.cpp
    src/py_procedure.cpp
    src/py_function.cpp
    #
    generated/py_event_args.cpp
    generated/py_type.cpp
    generated/py_simple_type.cpp
    generated/py_struct_type.cpp
    generated/py_type_manager.cpp
    generated/py_struct.cpp
    generated/py_struct_builder.cpp
)

add_library(${LIB_NAME} STATIC ${SRC_Headers} ${SRC_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::coretypes
    PRIVATE
        daq::coreobjects
        daq::opendaq
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
