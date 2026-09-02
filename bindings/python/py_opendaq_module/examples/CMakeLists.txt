# Not a test - a manually-run driver for examples/modules/python_plotter_module's live window
# (see run_live_example.cpp for what it does). It lives here, alongside py_opendaq_module, rather
# than under examples/modules/python_plotter_module/ itself, because it links
# daq::py_opendaq_module - self-gated behind OPENDAQ_GENERATE_PYTHON_MODULE_BINDINGS, see the
# parent CMakeLists.txt - which wouldn't exist yet if this were add_subdirectory'd from
# examples/ instead: the top-level CMakeLists.txt adds examples/ before bindings/. Not
# registered via add_test() since it opens an interactive window and blocks until closed.
set(PLOTTER_EXAMPLE_APP plotter_live_example)
set(PLOTTER_EXAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../examples/modules/python_plotter_module")

add_executable(${PLOTTER_EXAMPLE_APP} "${PLOTTER_EXAMPLE_DIR}/run_live_example.cpp")

set_target_properties(${PLOTTER_EXAMPLE_APP} PROPERTIES DEBUG_POSTFIX _debug)

target_link_libraries(${PLOTTER_EXAMPLE_APP} PRIVATE ${SDK_TARGET_NAMESPACE}::py_opendaq_module
                                                      daq::py_core_types
)

target_compile_definitions(${PLOTTER_EXAMPLE_APP} PRIVATE PLOTTER_MODULE_PATH="${PLOTTER_EXAMPLE_DIR}/plotter_module.py")
