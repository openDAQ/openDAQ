# Precompiled header helpers. Each is inert unless OPENDAQ_ENABLE_PCH is ON, so the default build
# is unaffected.

# Whether precompiled headers are in use. Not with the Intel compiler: icx emits the exception
# catchable-type records of a translation unit without the copy constructor when the throw was
# instantiated inside the precompiled header, so std::exception_ptr copies such exceptions bitwise
# and their message buffer is freed twice.
function(_opendaq_pch_in_use OUT_VAR)
    if (OPENDAQ_ENABLE_PCH AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
        set(${OUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Compiler options every target that uses a precompiled header needs.
function(_opendaq_pch_compile_options TARGET_NAME)
    # GCC loses the system-header status of declarations that come from a precompiled header, so
    # -Wdangling-reference then fires inside third-party code.
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
        target_compile_options(${TARGET_NAME} PRIVATE -Wno-dangling-reference)
    endif()
endfunction()

# Applies precompiled headers to a target when OPENDAQ_ENABLE_PCH is ON.
function(opendaq_target_pch TARGET_NAME)
    _opendaq_pch_in_use(PCH_IN_USE)
    if (PCH_IN_USE)
        target_precompile_headers(${TARGET_NAME} PRIVATE ${ARGN})
        _opendaq_pch_compile_options(${TARGET_NAME})
    endif()
endfunction()

# Reuses another target's precompiled header when OPENDAQ_ENABLE_PCH is ON. Both targets must compile
# with the same flags and definitions.
function(opendaq_target_pch_reuse TARGET_NAME DONOR_NAME)
    _opendaq_pch_in_use(PCH_IN_USE)
    if (PCH_IN_USE)
        target_precompile_headers(${TARGET_NAME} REUSE_FROM ${DONOR_NAME})
        _opendaq_pch_compile_options(${TARGET_NAME})
    endif()
endfunction()

# Precompiles a header set once per group: the first target to join builds the precompiled
# header and later members reuse it. Members must compile with the same flags and definitions,
# since the compiler validates the precompiled header against them.
function(opendaq_target_pch_group TARGET_NAME GROUP_NAME)
    _opendaq_pch_in_use(PCH_IN_USE)
    if (NOT PCH_IN_USE)
        return()
    endif()

    get_property(DONOR GLOBAL PROPERTY OPENDAQ_PCH_GROUP_${GROUP_NAME})
    if (DONOR)
        opendaq_target_pch_reuse(${TARGET_NAME} ${DONOR})
    else()
        opendaq_target_pch(${TARGET_NAME} ${ARGN})
        set_property(GLOBAL PROPERTY OPENDAQ_PCH_GROUP_${GROUP_NAME} ${TARGET_NAME})
    endif()
endfunction()
