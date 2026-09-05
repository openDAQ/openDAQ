# Build acceleration helpers. Each is inert unless its option is ON, so the default build is
# unaffected.
#
# Sharing a translation unit removes the per-file isolation that C++ sources normally rely on,
# so sources compiled in unity mode must follow three rules:
#
#   1. Types, constants and helper functions that are meant to be file-local belong in a
#      file-local namespace (or need per-file names). "static" and anonymous namespaces do not
#      help, because the merged sources share one translation unit.
#   2. using-directives stay at global scope, outside that namespace: code that reaches names
#      through them (for example ::event(...) resolving daq::event) breaks otherwise.
#   3. A source defining main(), or one that cannot share a translation unit for any other
#      reason, is excluded with the SKIP_UNITY_BUILD_INCLUSION source property.
#
# Explicit template specializations are the other thing to watch: the header declaring one has
# to be seen before anything instantiates the template. The target's precompiled header set
# guarantees that, and when no precompiled header is in use the same headers are placed at the
# top of each unity file instead.

# Whether precompiled headers are in use. Not with the Intel compiler: executables built from its
# precompiled headers fail to catch exceptions at run time.
function(_opendaq_pch_in_use OUT_VAR)
    if (OPENDAQ_ENABLE_PCH AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
        set(${OUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Without a precompiled header, the unity files include the same headers before every source.
function(_opendaq_unity_include_first TARGET_NAME)
    set(CODE "")
    foreach(HEADER IN LISTS ARGN)
        string(APPEND CODE "#include ${HEADER}\n")
    endforeach()
    set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY UNITY_BUILD_CODE_BEFORE_INCLUDE "${CODE}")
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
    else()
        _opendaq_unity_include_first(${TARGET_NAME} ${ARGN})
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
        _opendaq_unity_include_first(${TARGET_NAME} ${ARGN})
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

# Groups a target's sources into unity translation units.
function(_opendaq_target_unity TARGET_NAME BATCH_SIZE)
    if (NOT DEFINED BATCH_SIZE OR BATCH_SIZE STREQUAL "")
        set(BATCH_SIZE 12)
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        UNITY_BUILD ON
        UNITY_BUILD_BATCH_SIZE ${BATCH_SIZE}
    )

    if (MSVC)
        # Merged translation units run into the object file section limit (C1128).
        target_compile_options(${TARGET_NAME} PRIVATE /bigobj)
    endif()

    # GCC raises -Wsubobject-linkage for classes defined outside the main input file, which every
    # source is once it is included into a unity translation unit.
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${TARGET_NAME} PRIVATE -Wno-subobject-linkage)
    endif()
endfunction()

# Unity translation units for a test target (or for test-only support libraries), when
# OPENDAQ_ENABLE_UNITY_TESTS is ON.
function(opendaq_target_unity_tests TARGET_NAME)
    cmake_parse_arguments(UNITY "" "BATCH_SIZE" "" ${ARGN})

    if (OPENDAQ_ENABLE_UNITY_TESTS)
        _opendaq_target_unity(${TARGET_NAME} "${UNITY_BATCH_SIZE}")
    endif()
endfunction()

# Unity translation units for a generated language binding library, when
# OPENDAQ_ENABLE_UNITY_BINDINGS is ON.
function(opendaq_target_unity_bindings TARGET_NAME)
    cmake_parse_arguments(UNITY "" "BATCH_SIZE" "" ${ARGN})

    if (OPENDAQ_ENABLE_UNITY_BINDINGS)
        _opendaq_target_unity(${TARGET_NAME} "${UNITY_BATCH_SIZE}")
    endif()
endfunction()

# Unity translation units for a shipped library, when OPENDAQ_ENABLE_UNITY_LIBS is ON. Separate
# from the test and binding options because it trades incremental rebuild granularity in code
# that is edited, not just consumed.
function(opendaq_target_unity_libs TARGET_NAME)
    cmake_parse_arguments(UNITY "" "BATCH_SIZE" "" ${ARGN})

    if (OPENDAQ_ENABLE_UNITY_LIBS)
        _opendaq_target_unity(${TARGET_NAME} "${UNITY_BATCH_SIZE}")
    endif()
endfunction()
