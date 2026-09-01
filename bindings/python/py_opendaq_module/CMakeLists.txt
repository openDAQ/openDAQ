opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)

set(LIB_NAME py_opendaq_module)

# Embeds the same Module/FunctionBlock plugin base classes the real pip package ships (see
# bindings/python/package/opendaq/{module,function_block}.py) as C++ string constants, so
# python_opendaq_embed.cpp can exec them into the embedded interpreter's in-process "opendaq"
# module, which has no on-disk package to import them from.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../package/opendaq/module.py" OPENDAQ_MODULE_PY_SOURCE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../package/opendaq/function_block.py" OPENDAQ_FUNCTION_BLOCK_PY_SOURCE)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/py_opendaq_module/plugin_base_py.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated_include/py_opendaq_module/plugin_base_py.h"
    @ONLY
)

set(SRC_Headers
    include/py_opendaq_module/python_runtime.h
    include/py_opendaq_module/python_module.h
    include/py_opendaq_module/python_function_block.h
    include/py_opendaq_module/python_function_block_impl.h
)

set(SRC_Cpp
    src/python_runtime.cpp
    src/python_opendaq_embed.cpp
    src/python_module.cpp
    src/python_function_block_impl.cpp
)

add_library(${LIB_NAME} STATIC ${SRC_Headers} ${SRC_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::opendaq
        # Public headers use pybind11::object directly (PythonModule::PluginInfo, pyInstance),
        # and consumers embed the interpreter through this library, so both headers and the
        # embed runtime need to propagate to them, not stay an implementation detail.
        pybind11::headers
        pybind11::embed
    PRIVATE
        daq::py_core_types
        daq::py_core_objects
        daq::py_opendaq
        pybind11::lto
)

if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
    set_target_properties(${LIB_NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
endif()

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated_include/>
)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
