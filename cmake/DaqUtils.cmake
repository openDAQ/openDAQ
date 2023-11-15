include(openDAQUtils)
include(DaqInternal)

function(print_var VARIABLE_NAME)
    message(STATUS "${VARIABLE_NAME}: ${${VARIABLE_NAME}}")
endfunction()

function(get_current_folder OUTFOLDER)
    file(RELATIVE_PATH FOLDER ${PROJ_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    set(GROUP ${ARGV1})

    if (NOT GROUP)
        set(GROUP ${CMAKE_FOLDER})
    endif()

    if (GROUP)
        set(FOLDER "${GROUP}/${FOLDER}")
    endif()

    set(${OUTFOLDER} ${FOLDER} PARENT_SCOPE)
endfunction()

function(get_current_folder_name OUTFOLDER)
    get_filename_component(FOLDER ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    set(${OUTFOLDER} ${FOLDER} PARENT_SCOPE)
endfunction()

function(set_cmake_folder_context OUTFOLDER)
    get_current_folder_name(TARGET_FOLDER_NAME)

    if (ARGC GREATER 1)
        list(APPEND CMAKE_MESSAGE_CONTEXT ${ARGV1})
    else()
        list(APPEND CMAKE_MESSAGE_CONTEXT ${TARGET_FOLDER_NAME})
    endif()

    set(CMAKE_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT} PARENT_SCOPE)
    if (ARGC GREATER 1)
        set(CMAKE_FOLDER "${CMAKE_FOLDER}/${ARGV1}" PARENT_SCOPE)
    else()
        set(CMAKE_FOLDER "${CMAKE_FOLDER}/${TARGET_FOLDER_NAME}" PARENT_SCOPE)
    endif()
    set(${OUTFOLDER} ${TARGET_FOLDER_NAME} PARENT_SCOPE)
endfunction()

function(set_cmake_context)
    get_current_folder_name(TARGET_FOLDER_NAME)

    if (ARGC GREATER 1)
        list(APPEND CMAKE_MESSAGE_CONTEXT ${ARGV1})
    else()
        list(APPEND CMAKE_MESSAGE_CONTEXT ${TARGET_FOLDER_NAME})
    endif()

    set(CMAKE_MESSAGE_CONTEXT ${CMAKE_MESSAGE_CONTEXT} PARENT_SCOPE)
endfunction()

function(set_cmake_folder OUTFOLDER)
    get_current_folder_name(TARGET_FOLDER_NAME)
    set(CMAKE_FOLDER "${CMAKE_FOLDER}/${TARGET_FOLDER_NAME}" PARENT_SCOPE)
endfunction()

function(prepend_include SUBFOLDER SOURCE_FILES)
    list(TRANSFORM ${SOURCE_FILES} PREPEND "../include/${SUBFOLDER}/")
    set( ${SOURCE_FILES} ${${SOURCE_FILES}} PARENT_SCOPE )
endfunction()

function(create_version_header LIB_NAME)
    set(INCLUDE_FOLDER_NAME ${TARGET_FOLDER_NAME})

    set(options ONLY_RC NO_RC)
    set(oneValueArgs INCLUDE_FOLDER HEADER_NAME_PREFIX)
    set(multiValueArgs VARIANTS)
    cmake_parse_arguments(GENERATE_VERSION "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(INCLUDE_FOLDER_NAME ${TARGET_FOLDER_NAME})
    if (DEFINED GENERATE_VERSION_INCLUDE_FOLDER)
        set(INCLUDE_FOLDER_NAME ${GENERATE_VERSION_INCLUDE_FOLDER})
        set(HEADER_NAME_PREFIX "${TARGET_FOLDER_NAME}_")
    endif()

    if (DEFINED GENERATE_VERSION_HEADER_NAME_PREFIX)
        set(HEADER_NAME_PREFIX ${GENERATE_VERSION_HEADER_NAME_PREFIX})
    endif()
    string(STRIP "${HEADER_NAME_PREFIX}" HEADER_NAME_PREFIX)

    set(GENERATE_RC ON)
    set(GENERATE_HEADER ON)

    if (GENERATE_VERSION_NO_RC)
        set(GENERATE_RC OFF)
    endif()

    if (GENERATE_VERSION_ONLY_RC)
        set(GENERATE_HEADER OFF)
    endif()

    set(LIB_HEADERS_DIR ../include/${INCLUDE_FOLDER_NAME})

    opendaq_create_version_header(
        ${LIB_NAME}
        ${CMAKE_CURRENT_BINARY_DIR}/${LIB_HEADERS_DIR}
        "${HEADER_NAME_PREFIX}"
        ${GENERATE_RC}
        ${GENERATE_HEADER}
    )
endfunction()

function(check_if_files_exist DIR INPUT_FILES)
    foreach (INPUT_FILE ${${INPUT_FILES}})
        set(CUR_FILE ${DIR}/${INPUT_FILE})
        if (NOT EXISTS ${CUR_FILE})
            message(FATAL_ERROR "${CUR_FILE} does not exist")
        endif()
    endforeach()
endfunction(check_if_files_exist)

function(opendaq_create_dir DIR)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${DIR})
endfunction(opendaq_create_dir)

macro(add_application_option OPTION_VAR DESCRIPTION DEFAULT_VALUE)
    cmake_dependent_option(${OPTION_VAR} ${DESCRIPTION} ${DEFAULT_VALUE} "${SDK_OPTION_PREFIX}_ENABLE_APPLICATION" ON)
endmacro(add_application_option)

# opendaq_gather_flags
# Gathers all lists of flags for printing or manipulation
macro(opendaq_gather_flags with_linker result)
    set(${result} "")
    # add the main flags without a config
    list(APPEND ${result} CMAKE_C_FLAGS)
    list(APPEND ${result} CMAKE_CXX_FLAGS)
    if(${with_linker})
        list(APPEND ${result} CMAKE_EXE_LINKER_FLAGS)
        list(APPEND ${result} CMAKE_MODULE_LINKER_FLAGS)
        list(APPEND ${result} CMAKE_SHARED_LINKER_FLAGS)
        list(APPEND ${result} CMAKE_STATIC_LINKER_FLAGS)
    endif()

    if("${CMAKE_CONFIGURATION_TYPES}" STREQUAL "" AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "")
        # handle single config generators - like makefiles/ninja - when CMAKE_BUILD_TYPE is set
        string(TOUPPER ${CMAKE_BUILD_TYPE} config)
        list(APPEND ${result} CMAKE_C_FLAGS_${config})
        list(APPEND ${result} CMAKE_CXX_FLAGS_${config})
        if(${with_linker})
            list(APPEND ${result} CMAKE_EXE_LINKER_FLAGS_${config})
            list(APPEND ${result} CMAKE_MODULE_LINKER_FLAGS_${config})
            list(APPEND ${result} CMAKE_SHARED_LINKER_FLAGS_${config})
            list(APPEND ${result} CMAKE_STATIC_LINKER_FLAGS_${config})
        endif()
    else()
        # handle multi config generators (like msvc, xcode)
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${config} config)
            list(APPEND ${result} CMAKE_C_FLAGS_${config})
            list(APPEND ${result} CMAKE_CXX_FLAGS_${config})
            if(${with_linker})
                list(APPEND ${result} CMAKE_EXE_LINKER_FLAGS_${config})
                list(APPEND ${result} CMAKE_MODULE_LINKER_FLAGS_${config})
                list(APPEND ${result} CMAKE_SHARED_LINKER_FLAGS_${config})
                list(APPEND ${result} CMAKE_STATIC_LINKER_FLAGS_${config})
            endif()
        endforeach()
    endif()
endmacro()

# opendaq_set_runtime
# Sets the runtime (static/dynamic) for msvc/gcc
macro(opendaq_set_runtime)
    cmake_parse_arguments(ARG "STATIC;DYNAMIC" "" "" ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "unrecognized arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" STREQUAL "")
        message(AUTHOR_WARNING "opendaq_set_runtime() does not support clang yet!")
    endif()

    opendaq_gather_flags(0 flags_configs)

    # add/replace the flags
    # note that if the user has messed with the flags directly this function might fail
    # - for example if with MSVC and the user has removed the flags - here we just switch/replace them
    if("${ARG_STATIC}")
        foreach(flags ${flags_configs})
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                if(NOT ${flags} MATCHES "-static-libstdc\\+\\+")
                    set(${flags} "${${flags}} -static-libstdc++")
                endif()
                if(NOT ${flags} MATCHES "-static-libgcc")
                    set(${flags} "${${flags}} -static-libgcc")
                endif()
            elseif(MSVC)
                if(${flags} MATCHES "/MD")
                    string(REGEX REPLACE "/MD" "/MT" ${flags} "${${flags}}")
                endif()
            endif()
        endforeach()
    elseif("${ARG_DYNAMIC}")
        foreach(flags ${flags_configs})
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                if(${flags} MATCHES "-static-libstdc\\+\\+")
                    string(REGEX REPLACE "-static-libstdc\\+\\+" "" ${flags} "${${flags}}")
                endif()
                if(${flags} MATCHES "-static-libgcc")
                    string(REGEX REPLACE "-static-libgcc" "" ${flags} "${${flags}}")
                endif()
            elseif(MSVC)
                if(${flags} MATCHES "/MT")
                    string(REGEX REPLACE "/MT" "/MD" ${flags} "${${flags}}")
                endif()
            endif()
        endforeach()
    endif()
endmacro()

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

macro(is_git_repository_root DIRECTORY_PATH FUNC_RESULT)
    execute_process(COMMAND git rev-parse --resolve-git-dir ${${DIRECTORY_PATH}}/.git
                    OUTPUT_QUIET
                    ERROR_QUIET
                    RESULT_VARIABLE GIT_RESULT)
    if (GIT_RESULT EQUAL "0")
        set(${FUNC_RESULT} TRUE)
    else()
        set(${FUNC_RESULT} FALSE)
    endif()
endmacro()

function(opendaq_set_target_postfixes _TARGET_NAME)
    if (BUILD_64Bit)
        set(_DEBUG_SUFFIX "64_debug")
        set(_RELEASE_SUFFIX "64")
    else()
        set(_DEBUG_SUFFIX "_debug")
        set(_RELEASE_SUFFIX "")
    endif()

    set_target_properties(${_TARGET_NAME} PROPERTIES DEBUG_POSTFIX ${_DEBUG_SUFFIX})

    if (_RELEASE_SUFFIX)
        set_target_properties(${_TARGET_NAME} PROPERTIES RELEASE_POSTFIX ${_RELEASE_SUFFIX})
        set_target_properties(${_TARGET_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX ${_RELEASE_SUFFIX})
        set_target_properties(${_TARGET_NAME} PROPERTIES MINSIZEREL_POSTFIX ${_RELEASE_SUFFIX})
    endif()
endfunction()

function(use_compiler_cache)
    if((NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR) OR (NOT OPENDAQ_USE_CCACHE))
        return()
    endif()

    find_program(CCACHE_PROGRAM ccache)
    if(NOT CCACHE_PROGRAM)
        message(STATUS "CCache not found!")
        return()
    endif()

    set(ccacheEnv
        CCACHE_BASEDIR=${CMAKE_SOURCE_DIR}
        CCACHE_CPP2=true
        CCACHE_SLOPPINESS=pch_defines,time_macros
    )

    if(CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
#    if(CMAKE_GENERATOR MATCHES "Ninja|Makefiles|Visual Studio")
#      - currently only Ninja / Makefiles supports MSVC
        message(STATUS "Using CCache: ${CCACHE_PROGRAM}")

        foreach(lang IN ITEMS C CXX OBJC OBJCXX CUDA)
            set(CMAKE_${lang}_COMPILER_LAUNCHER
                ${CMAKE_COMMAND} -E env ${ccacheEnv} ${CCACHE_PROGRAM}
                PARENT_SCOPE
            )
        endforeach()
    elseif(CMAKE_GENERATOR STREQUAL Xcode)
        message(STATUS "Using CCache with XCode: ${CCACHE_PROGRAM}")

        foreach(lang IN ITEMS C CXX)
            set(launch${lang} ${CMAKE_BINARY_DIR}/launch-${lang})
            file(WRITE ${launch${lang}} "#!/bin/bash\n\n")

            foreach(keyVal IN LISTS ccacheEnv)
                file(APPEND ${launch${lang}} "export ${keyVal}\n")
            endforeach()

            file(APPEND ${launch${lang}}
                "exec \"${CCACHE_PROGRAM}\" \"${CMAKE_${lang}_COMPILER}\" \"$@\"\n")

            execute_process(COMMAND chmod a+rx ${launch${lang}})
        endforeach()

        set(CMAKE_XCODE_ATTRIBUTE_CC ${launchC} PARENT_SCOPE)
        set(CMAKE_XCODE_ATTRIBUTE_CXX ${launchCXX} PARENT_SCOPE)
        set(CMAKE_XCODE_ATTRIBUTE_LD ${launchC} PARENT_SCOPE)
        set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS ${launchCXX} PARENT_SCOPE)
    endif()
endfunction()

function(get_custom_fetch_content_params LIBRARY_NAME OUTPARAM)
    set(FC_SOURCE_DIR ${FETCHCONTENT_EXTERNALS_DIR}/src)
    set(FC_SUBBUILD_DIR ${FETCHCONTENT_EXTERNALS_DIR}/subbuild/${CMAKE_GENERATOR}/${CMAKE_CXX_COMPILER_ID})

    if (CMAKE_GENERATOR_PLATFORM)
        set(FC_SUBBUILD_DIR ${FC_SUBBUILD_DIR}/${CMAKE_GENERATOR_PLATFORM})
    endif()

    set(${OUTPARAM}
        DOWNLOAD_DIR ${FC_SOURCE_DIR}
        SOURCE_DIR ${FC_SOURCE_DIR}/${LIBRARY_NAME}
        SUBBUILD_DIR ${FC_SUBBUILD_DIR}/${LIBRARY_NAME}
        #EXCLUDE_FROM_ALL ON
        PARENT_SCOPE
    )
endfunction()
