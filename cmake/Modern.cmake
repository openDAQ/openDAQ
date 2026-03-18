# Deprecated aliases — redirected to opendaq-cmake-utils (openDAQModernCMakeUtils.cmake)
# Use opendaq_set_cmake_mode() / opendaq_get_cmake_mode() directly.

function(set_mode MODE)
    message(DEPRECATION "set_mode() is deprecated. Use opendaq_set_cmake_mode() instead.")
    opendaq_set_cmake_mode(${MODE})
endfunction()

function(get_mode MODE)
    message(DEPRECATION "get_mode() is deprecated. Use opendaq_get_cmake_mode() instead.")
    opendaq_get_cmake_mode(${MODE})
    set(${MODE} ${${MODE}} PARENT_SCOPE)
endfunction()
