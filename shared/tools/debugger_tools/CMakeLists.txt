opendaq_set_cmake_context()

message(STATUS "Debug extensions enabled")

set(OPENDAQ_GDB_PY_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()

if(MSVC OR IS_MULTI_CONFIG)
    set(OutputBinDir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
else()
    set(OutputBinDir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/gdbinit.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gdbinit.intermediate"
    @ONLY
)

file(GENERATE
    OUTPUT "${OutputBinDir}/.gdbinit"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/gdbinit.intermediate"
)