# ----------------------------------------------------------------------
# Defines a deprecated function alias
# Parameters:
#   OLD_NAME - old function name (deprecated)
#   NEW_NAME - new function name to call
# ----------------------------------------------------------------------
macro(define_deprecated_function_alias OLD_NAME NEW_NAME)
    if(NOT COMMAND ${NEW_NAME})
        message(FATAL_ERROR
            "Deprecated alias ${OLD_NAME}() refers to missing command "
            "'${NEW_NAME}()'."
        )
    endif()
    function(${OLD_NAME})
        # warn only once
        if(NOT DEFINED _${OLD_NAME}_DEPRECATED_WARNED)
            set(_${OLD_NAME}_DEPRECATED_WARNED TRUE CACHE INTERNAL "")
            message(DEPRECATION "${OLD_NAME}() is deprecated. Use ${NEW_NAME}() instead.")
        endif()
        ${NEW_NAME}(${ARGV})
    endfunction()
endmacro()

# Functions
define_deprecated_function_alias(print_var 				opendaq_print_var)
define_deprecated_function_alias(get_current_folder 			opendaq_get_current_folder)
define_deprecated_function_alias(set_cmake_folder_context 		opendaq_set_cmake_folder_context)
define_deprecated_function_alias(set_cmake_context 			opendaq_set_cmake_context)
define_deprecated_function_alias(set_cmake_folder 			opendaq_set_cmake_folder)
define_deprecated_function_alias(prepend_include 			opendaq_prepend_include)
define_deprecated_function_alias(create_version_header 		opendaq_generate_version_header)
define_deprecated_function_alias(check_if_files_exist 		opendaq_check_if_files_exist)
define_deprecated_function_alias(dump_cmake_variables 		opendaq_dump_cmake_variables)
define_deprecated_function_alias(use_compiler_cache 			opendaq_use_compiler_cache)
define_deprecated_function_alias(get_custom_fetch_content_params 	opendaq_get_custom_fetch_content_params)
define_deprecated_function_alias(add_cmake_targets 			opendaq_add_cmake_targets)


# ----------------------------------------------------------------------
# Defines a deprecated macro alias
# Parameters:
#   OLD_NAME - old macro name (deprecated)
#   NEW_NAME - new macro name to call
# ----------------------------------------------------------------------
macro(define_deprecated_macro_alias OLD_NAME NEW_NAME)
    if(NOT COMMAND ${NEW_NAME})
        message(FATAL_ERROR
            "Deprecated alias ${OLD_NAME}() refers to missing command "
            "'${NEW_NAME}()'."
        )
    endif()
    macro(${OLD_NAME})
        # warn only once
        if(NOT DEFINED _${OLD_NAME}_DEPRECATED_WARNED)
            set(_${OLD_NAME}_DEPRECATED_WARNED TRUE CACHE INTERNAL "")
            message(DEPRECATION "${OLD_NAME}() is deprecated. Use ${NEW_NAME}() instead.")
        endif()
        ${NEW_NAME}(${ARGV})
    endmacro()
endmacro()

# Macro
define_deprecated_macro_alias	 (is_git_repository_root 		opendaq_is_git_repository_root)
define_deprecated_macro_alias	 (add_application_option 		opendaq_add_application_option)

