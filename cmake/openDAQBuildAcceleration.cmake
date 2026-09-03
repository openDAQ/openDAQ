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
# to be seen before anything instantiates the template, which the target's precompiled header
# set is the reliable way to guarantee.

# Applies precompiled headers to a target when OPENDAQ_ENABLE_PCH is ON.
function(opendaq_target_pch TARGET_NAME)
    if (OPENDAQ_ENABLE_PCH)
        target_precompile_headers(${TARGET_NAME} PRIVATE ${ARGN})
    endif()
endfunction()

# Precompiles a header set once per group: the first target to join builds the precompiled
# header and later members reuse it. Members must compile with the same flags and definitions,
# since the compiler validates the precompiled header against them.
function(opendaq_target_pch_group TARGET_NAME GROUP_NAME)
    if (NOT OPENDAQ_ENABLE_PCH)
        return()
    endif()

    get_property(DONOR GLOBAL PROPERTY OPENDAQ_PCH_GROUP_${GROUP_NAME})
    if (DONOR)
        target_precompile_headers(${TARGET_NAME} REUSE_FROM ${DONOR})
    else()
        target_precompile_headers(${TARGET_NAME} PRIVATE ${ARGN})
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
