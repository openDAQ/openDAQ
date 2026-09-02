opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)

set(CORE_LIB_NAME py_opendaq_module_core)
set(LIB_NAME py_opendaq_module)

set(SRC_Headers
    include/py_opendaq_module/python_runtime.h
    include/py_opendaq_module/python_module.h
    include/py_opendaq_module/python_function_block.h
    include/py_opendaq_module/python_function_block_impl.h
)

set(CORE_SRC_Cpp
    src/python_runtime.cpp
    src/python_module.cpp
    src/python_function_block_impl.cpp
)

# PythonRuntime/PythonModule/PythonFunctionBlockImpl on their own, deliberately without
# python_opendaq_embed.cpp - usable both by a C++ host that embeds Python (which links
# ${LIB_NAME} below, adding that embedded-module registration on top of this) and by a host
# that already *is* Python, i.e. py_opendaq_daq (bindings/python/py_opendaq_daq/CMakeLists.txt),
# which links this target directly. py_opendaq_daq must not also pull in
# python_opendaq_embed.cpp's PYBIND11_EMBEDDED_MODULE(opendaq, ...) block: that registers every
# wrapped type a second time via wrapDaqComponentCoreTypes/CoreObjects/OpenDaq, which pybind11
# rejects outright (a C++ type can only be registered with `py::class_` once per interpreter) -
# and it does not need to, since PythonRuntime's attached-interpreter branch (see
# python_runtime.h) resolves `import opendaq` to the very module py_opendaq_daq itself already
# is, not a from-scratch embedded copy.
add_library(${CORE_LIB_NAME} STATIC ${SRC_Headers} ${CORE_SRC_Cpp})
add_library(${SDK_TARGET_NAMESPACE}::${CORE_LIB_NAME} ALIAS ${CORE_LIB_NAME})

target_link_libraries(${CORE_LIB_NAME}
    PUBLIC
        daq::opendaq
        # Public headers use pybind11::object directly (PythonModule::PluginInfo, pyInstance),
        # so Python.h needs to be findable here - pybind11::module (not pybind11::headers, which
        # doesn't provide it) does that. Not pybind11::embed: PythonRuntime references
        # pybind11::scoped_interpreter's type (for its owning-interpreter branch) but never
        # requires it to actually be linked as an embedding target - the Py_Initialize()-family
        # symbols it calls only need to be resolvable at runtime, which whichever final target
        # consumes this (an embedding host linking pybind11::embed itself, or a Python extension
        # module like py_opendaq_daq, where they resolve against the host process's own
        # already-loaded libpython) already guarantees on its own. pybind11::module matches
        # exactly what an extension-module consumer like py_opendaq_daq needs anyway - headers,
        # plus libpython linked only where that platform's extension modules require it
        # (Windows/Cygwin/Android) - so this stays correct for it without extra conditions here.
        pybind11::module
    PRIVATE
        daq::py_core_types
        daq::py_core_objects
        daq::py_opendaq
        pybind11::lto
)

if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
    set_target_properties(${CORE_LIB_NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
endif()

target_include_directories(${CORE_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

# Everything below is the embedded-interpreter half, self-gated behind
# OPENDAQ_GENERATE_PYTHON_MODULE_BINDINGS (this whole subdirectory used to be added only under
# that option - the ${CORE_LIB_NAME} target above now needs to stay available unconditionally
# for py_opendaq_daq to link, see its CMakeLists.txt, so the option is checked here instead).
if (OPENDAQ_GENERATE_PYTHON_MODULE_BINDINGS)

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

# The embedded-interpreter half: adds Py_Initialize()-owning support (PythonRuntime's owning
# branch) and the in-process "opendaq" module python_opendaq_embed.cpp registers, for a C++ host
# with no Python of its own - see PythonModule(context, pathToPythonFile)'s doc comment.
add_library(${LIB_NAME} STATIC src/python_opendaq_embed.cpp)
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::${CORE_LIB_NAME}
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
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated_include/>
)

# python_opendaq_embed.cpp's PYBIND11_EMBEDDED_MODULE registration is a static-initializer side
# effect - nothing else in this whole library defines or references any other symbol from it, so
# a normal static-archive link would silently drop it (a linker only pulls an object file in from
# an archive if something already-included references an unresolved symbol it defines). Force it
# in unconditionally instead, for any executable that links ${LIB_NAME}: PUBLIC + INTERFACE so it
# applies to consuming executables' own link lines, not just this library's.
if(APPLE)
    target_link_options(${LIB_NAME} INTERFACE "-Wl,-force_load,$<TARGET_FILE:${LIB_NAME}>")
elseif(UNIX)
    target_link_options(${LIB_NAME} INTERFACE
        "-Wl,--whole-archive" "$<TARGET_FILE:${LIB_NAME}>" "-Wl,--no-whole-archive")
elseif(MSVC)
    target_link_options(${LIB_NAME} INTERFACE "/WHOLEARCHIVE:$<TARGET_FILE:${LIB_NAME}>")
endif()

add_subdirectory(examples)

if (OPENDAQ_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

endif() # OPENDAQ_GENERATE_PYTHON_MODULE_BINDINGS
