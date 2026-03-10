# ----------------------------------------------------------------------
# Prints a deprecation for aliased function/macro once if called
# ----------------------------------------------------------------------
function(opendaq_show_deprecated_message_once ALIASED_NAME ORIGINAL_NAME)
    if(NOT DEFINED _${ALIASED_NAME}_DEPRECATED_WARNED)
        set(_${ALIASED_NAME}_DEPRECATED_WARNED TRUE CACHE INTERNAL "")
        message(DEPRECATION "${ALIASED_NAME}() is deprecated. Use ${ORIGINAL_NAME}() instead.")
    endif()
endfunction()

# ----------------------------------------------------------------------
# Define a deprecated function/macro aliases
# ----------------------------------------------------------------------
macro(print_var)
    opendaq_show_deprecated_message_once(print_var opendaq_print_var)
    opendaq_print_var(${ARGV})
endmacro()

macro(get_current_folder)
    opendaq_show_deprecated_message_once(get_current_folder opendaq_get_current_folder)
    opendaq_get_current_folder(${ARGV})
endmacro()

macro(get_current_folder_name)
    opendaq_show_deprecated_message_once(get_current_folder_name opendaq_get_current_folder_name)
    opendaq_get_current_folder_name(${ARGV})
endmacro()

macro(set_cmake_folder_context)
    opendaq_show_deprecated_message_once(set_cmake_folder_context opendaq_set_cmake_folder_context)
    opendaq_set_cmake_folder_context(${ARGV})
endmacro()

macro(set_cmake_context)
    opendaq_show_deprecated_message_once(set_cmake_context opendaq_set_cmake_context)
    opendaq_set_cmake_context(${ARGV})
endmacro()

macro(set_cmake_folder)
    opendaq_show_deprecated_message_once(set_cmake_folder opendaq_set_cmake_folder)
    opendaq_set_cmake_folder(${ARGV})
endmacro()

macro(prepend_include)
    opendaq_show_deprecated_message_once(prepend_include opendaq_prepend_include)
    opendaq_prepend_include(${ARGV})
endmacro()

macro(create_version_header)
    opendaq_show_deprecated_message_once(create_version_header opendaq_generate_version_header)
    opendaq_generate_version_header(${ARGV})
endmacro()

macro(check_if_files_exist)
    opendaq_show_deprecated_message_once(check_if_files_exist opendaq_check_if_files_exist)
    opendaq_check_if_files_exist(${ARGV})
endmacro()

macro(dump_cmake_variables)
    opendaq_show_deprecated_message_once(dump_cmake_variables opendaq_dump_cmake_variables)
    opendaq_dump_cmake_variables(${ARGV})
endmacro()

macro(use_compiler_cache)
    opendaq_show_deprecated_message_once(use_compiler_cache opendaq_use_compiler_cache)
    opendaq_use_compiler_cache(${ARGV})
endmacro()

macro(get_custom_fetch_content_params)
    opendaq_show_deprecated_message_once(get_custom_fetch_content_params opendaq_get_custom_fetch_content_params)
    opendaq_get_custom_fetch_content_params(${ARGV})
endmacro()

macro(add_cmake_targets)
    opendaq_show_deprecated_message_once(add_cmake_targets opendaq_add_cmake_targets)
    opendaq_add_cmake_targets(${ARGV})
endmacro()

macro(is_git_repository_root)
    opendaq_show_deprecated_message_once(is_git_repository_root opendaq_is_git_repository_root)
    opendaq_is_git_repository_root(${ARGV})
endmacro()

macro(add_application_option)
    opendaq_show_deprecated_message_once(add_application_option opendaq_add_application_option)
    opendaq_add_application_option(${ARGV})
endmacro()

