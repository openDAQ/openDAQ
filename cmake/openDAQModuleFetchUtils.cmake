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

    opendaq_set_cmake_folder_context(TARGET_FOLDER_NAME)
    opendaq_get_custom_fetch_content_params(${FETCHED_MODULE_NAME} FC_PARAMS)

    FetchContent_Declare(
        ${FETCHED_MODULE_NAME}
        GIT_REPOSITORY ${FETCHED_MODULE_GIT_REPOSITORY}
        GIT_TAG        ${FETCHED_MODULE_GIT_REF}
        GIT_PROGRESS   ON
        GIT_SHALLOW    ${PARAMS_GIT_SHALLOW}
        GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
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
        message(WARNING "${FETCHED_MODULE_NAME} module binary directory ${FETCHED_MODULE_BINARY_DIR} is invalid")
    endif()
    message(STATUS "opendaq_fetch_module(${FETCHED_MODULE_NAME}) BINARY DIR: ${FETCHED_MODULE_BINARY_DIR}")
endmacro()

macro(opendaq_register_module_options MODULE_DIR)
    include("${MODULE_DIR}/cmake/ModuleOptions.cmake" OPTIONAL)
endmacro()

macro(opendaq_register_module_boost_dependencies MODULE_DIR)
    set(OPENDAQ_MODULE_BOOSTLIB_FILE "${MODULE_DIR}/external/boost/Boost.cmake")
    if(EXISTS "${OPENDAQ_MODULE_BOOSTLIB_FILE}")
        message(STATUS "Add boost required libraries listed in ${OPENDAQ_MODULE_BOOSTLIB_FILE}")
        include("${OPENDAQ_MODULE_BOOSTLIB_FILE}")
    endif()
endmacro()
