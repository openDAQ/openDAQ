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

# 1. Find the GDB executable on the system
find_program(GDB_EXECUTABLE gdb)

if(GDB_EXECUTABLE)
    message(STATUS "Checking GDB auto-load safe-path...")

    # 2. Ask GDB to print its safe-path setting
    execute_process(
        COMMAND ${GDB_EXECUTABLE} --batch -ex "show auto-load safe-path"
        OUTPUT_VARIABLE GDB_SAFE_PATH_OUTPUT
        RESULT_VARIABLE GDB_SAFE_PATH_RESULT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(GDB_SAFE_PATH_RESULT EQUAL 0)
        # GDB output usually looks like: 
        # "List of directories from which it is safe to auto-load files is /."
        
        # 3. Check if the default allow-all ("/") is set
        string(FIND "${GDB_SAFE_PATH_OUTPUT}" "is /" ALLOW_ALL_INDEX)
        
        # 4. Check if our specific base build directory is in the list
        string(FIND "${GDB_SAFE_PATH_OUTPUT}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" SPECIFIC_DIR_INDEX)

        if(ALLOW_ALL_INDEX GREATER -1)
            message(STATUS "GDB auto-load safe-path allows all directories (/).")
        elseif(SPECIFIC_DIR_INDEX GREATER -1)
            message(STATUS "GDB auto-load safe-path explicitly allows the build directory.")
        else()  
            # 5. Warn the user if it's not allowed
            message(WARNING 
                "GDB will likely block auto-loading the generated .gdbinit!\n"
                "To fix this permanently, add this to your global ~/.gdbinit:\n"
                "  add-auto-load-safe-path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/\n"
                "\n"
                "Alternatively, you can load it manually in two ways:\n"
                "1. When launching GDB from the terminal:\n"
                "   gdb -x ${OutputBinDir}/.gdbinit ./YourExecutable\n"
                "2. From inside an active GDB session:\n"
                "   (gdb) source ${OutputBinDir}/.gdbinit\n"
            )
        endif()
    else()
        message(STATUS "Could not execute GDB to check safe-path.")
    endif()
else()
    message(STATUS "GDB executable not found; skipping safe-path check.")
endif()