set(LIB_NAME py_opendaq_daq)

pybind11_add_module(${LIB_NAME}
    MODULE
        src/opendaq_daq.cpp
)

target_link_libraries(${LIB_NAME}
    PRIVATE
        daq::py_core_types
        daq::py_core_objects
        daq::py_opendaq
)

set_target_properties(${LIB_NAME}
    PROPERTIES
        OUTPUT_NAME "opendaq"
        DEBUG_POSTFIX ""
)

if(UNIX)
    if(APPLE)
        set_target_properties(${LIB_NAME} PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    else()
        set_target_properties(${LIB_NAME}
            PROPERTIES
                BUILD_RPATH_USE_ORIGIN TRUE
        )
    endif()
endif()


if (OPENDAQ_GENERATE_PYTHON_BINDINGS_STUBS)  
    execute_process(COMMAND ${Python_EXECUTABLE} -m pybind11_stubgen -h RESULT_VARIABLE PYBIND11_STUBGEN_RESULT OUTPUT_QUIET)
    if (${PYBIND11_STUBGEN_RESULT} GREATER 0)
        message(FATAL_ERROR "Failed to execute pybind11-stubgen python module. Make sure it is installed with `pip install pybind11-stubgen` or `python -m pip install pybind11_stubgen`.")    
    endif()

    if(MSVC OR GENERATOR_IS_MULTI_CONFIG)
        set(OutputBinDir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
    else()
        set(OutputBinDir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif()
    
    add_custom_command(TARGET ${LIB_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${OutputBinDir} -- ${Python_EXECUTABLE} -m pybind11_stubgen opendaq -o .
        WORKING_DIRECTORY ${OutputBinDir}
        COMMENT "Generate python bindings stubs (.pyi)")
endif()