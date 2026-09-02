# Applies precompiled headers to a target when OPENDAQ_ENABLE_PCH is ON.
function(opendaq_target_pch TARGET_NAME)
    if (OPENDAQ_ENABLE_PCH)
        target_precompile_headers(${TARGET_NAME} PRIVATE ${ARGN})
    endif()
endfunction()
