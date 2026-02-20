# opendaq_fetch_module(
#     NAME                module_name
#     GIT_REPOSITORY      https://github.com/org/repo.git
#     GIT_REF             v1.2.3 | branch-name
#     [ GIT_SHALLOW       ON|OFF ]
# )
#
macro(opendaq_fetch_module)

    cmake_parse_arguments(FETCHED_MODULE
        ""
        "NAME;GIT_REPOSITORY;GIT_REF;GIT_SHALLOW"
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
    )
    if(NOT FETCHED_MODULE_POPULATED)
        message(FATAL_ERROR "Fail to populate ${FETCHED_MODULE_NAME} module")
    endif()
endmacro()
