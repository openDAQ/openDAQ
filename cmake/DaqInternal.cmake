function(opendaq_forward_include_headers LIB_NAME MODULES)
    foreach(MODULE_NAME ${${MODULES}})
        target_include_directories(
            ${LIB_NAME}
            INTERFACE $<TARGET_PROPERTY:${MODULE_NAME},INTERFACE_INCLUDE_DIRECTORIES>
        )
    endforeach()
endfunction()

function(opendaq_prepare_test_runner TEST_TARGET)
    set(options "")
    set(oneValueArgs FOR)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(RUNNER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED RUNNER_FOR)
        message(FATAL_ERROR "opendaq_prepare_test_runner() requires the module under test to be specified with FOR.")
    endif()

    if (NOT DEFINED RUNNER_SOURCES)
        message(FATAL_ERROR "No SOURCES given to opendaq_prepare_test_runner().")
    endif()

    set(TEST_RUNNER test_${RUNNER_FOR})
    add_executable(${TEST_RUNNER} ${RUNNER_SOURCES})

    set_target_properties(${TEST_RUNNER} PROPERTIES DEBUG_POSTFIX _debug)

    target_link_libraries(${TEST_RUNNER} PRIVATE ${MAIN_TARGET}
                                                 ${SDK_TARGET_NAMESPACE}::test_utils
    )

    if (MSVC)
        target_compile_options(${TEST_RUNNER} PRIVATE /wd4100)
    endif()

    set(${TEST_TARGET} ${TEST_RUNNER} PARENT_SCOPE)
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

function(opendaq_create_dir DIR)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${DIR})
endfunction(opendaq_create_dir)

macro(add_application_option OPTION_VAR DESCRIPTION DEFAULT_VALUE) # not used - but might be used internally by opendaq
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

function(add_cmake_targets DIR INOUT_TARGET_LIST)
    #prerequisite is that the directory has already been added/processed

    set(_TARGETS ${${INOUT_TARGET_LIST}})

    get_property(_TGTS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND _TARGETS ${_TGTS})
    #foreach(_TGT IN LISTS _TGTS)
    #    #message(STATUS "Adding target: ${_TGT}")
    #endforeach()

    #recursively through all sub directories
    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR IN LISTS SUBDIRS)
        add_cmake_targets("${SUBDIR}" _TARGETS)
    endforeach()

    # Set output argument
    set(${INOUT_TARGET_LIST} ${_TARGETS} PARENT_SCOPE)
endfunction(add_cmake_targets)

# opendaq_fetch_module(
#     NAME                module_name
#     GIT_REPOSITORY      https://github.com/org/repo.git
#     GIT_REF             v1.2.3 | branch-name
#     [ GIT_SHALLOW       ON|OFF ]
#     [ EXCLUDE_FROM_ALL  ON|OFF ]
# )
#
macro(opendaq_fetch_module)

    cmake_parse_arguments(FETCHED_MODULE
        ""
        "NAME;GIT_REPOSITORY;GIT_REF;GIT_SHALLOW;EXCLUDE_FROM_ALL"
        ""
        ${ARGN}
    )

    if (NOT FETCHED_MODULE_NAME)
        message(FATAL_ERROR "opendaq_fetch_module: NAME is required")
    endif()

    if (NOT FETCHED_MODULE_GIT_REPOSITORY)
        message(FATAL_ERROR "opendaq_fetch_module(${FETCHED_MODULE_NAME}): GIT_REPOSITORY is required")
    endif()

    if (NOT FETCHED_MODULE_GIT_REF)
        message(FATAL_ERROR "opendaq_fetch_module(${FETCHED_MODULE_NAME}): GIT_REF is required")
    endif()

    set(PARAMS_GIT_SHALLOW ON)
    if (DEFINED FETCHED_MODULE_GIT_SHALLOW)
        set(PARAMS_GIT_SHALLOW ${FETCHED_MODULE_GIT_SHALLOW})
    endif()

    set(PARAMS_EXCLUDE_FROM_ALL OFF)
    if (DEFINED FETCHED_MODULE_EXCLUDE_FROM_ALL)
        set(PARAMS_EXCLUDE_FROM_ALL ${FETCHED_MODULE_EXCLUDE_FROM_ALL})
    endif()

    opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)
    opendaq_get_custom_fetch_content_params(${FETCHED_MODULE_NAME} FC_PARAMS)

    FetchContent_Declare(
        ${FETCHED_MODULE_NAME}
        GIT_REPOSITORY ${FETCHED_MODULE_GIT_REPOSITORY}
        GIT_TAG        ${FETCHED_MODULE_GIT_REF}
        GIT_PROGRESS   ON
        GIT_SHALLOW    ${PARAMS_GIT_SHALLOW}
        GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
        EXCLUDE_FROM_ALL ${PARAMS_EXCLUDE_FROM_ALL}
        ${FC_PARAMS}
    )

    FetchContent_Populate(${FETCHED_MODULE_NAME})

    FetchContent_GetProperties(
        ${FETCHED_MODULE_NAME}
        POPULATED  FETCHED_MODULE_POPULATED
        SOURCE_DIR FETCHED_MODULE_SOURCE_DIR
        BINARY_DIR FETCHED_MODULE_BINARY_DIR
    )

    if(NOT FETCHED_MODULE_POPULATED)
        message(FATAL_ERROR "Fail to populate ${FETCHED_MODULE_NAME} module")
    endif()

    if(NOT FETCHED_MODULE_SOURCE_DIR OR NOT IS_DIRECTORY "${FETCHED_MODULE_SOURCE_DIR}")
        message(FATAL_ERROR "${FETCHED_MODULE_NAME} module source directory ${FETCHED_MODULE_SOURCE_DIR} is invalid")
    endif()
    message(STATUS "opendaq_fetch_module(${FETCHED_MODULE_NAME}) SORCE DIR: ${FETCHED_MODULE_SOURCE_DIR}")

    if(NOT FETCHED_MODULE_BINARY_DIR OR NOT IS_DIRECTORY "${FETCHED_MODULE_BINARY_DIR}")
        message(FATAL_ERROR "${FETCHED_MODULE_NAME} module binary directory ${FETCHED_MODULE_BINARY_DIR} is invalid")
    endif()
    message(STATUS "opendaq_fetch_module(${FETCHED_MODULE_NAME}) BINARY DIR: ${FETCHED_MODULE_BINARY_DIR}")
endmacro()
