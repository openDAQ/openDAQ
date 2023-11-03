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
    set_target_properties(${LIB_NAME}
        PROPERTIES
            BUILD_RPATH_USE_ORIGIN ON
    )
endif()
