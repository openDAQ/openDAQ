set(opendaq_dependency__internal_dir ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "")

#
# opendaq_check_dependency(
#     VAR var_to_set
#     EXPECTED_TARGET target::target
#     EXPECTED_COMMAND command
# )
#
# This macro ensures a specified target and/or command are defined, and sets variable 'var_to_set'
# to 0 or 1 accordingly. Either EXPECTED_TARGET or EXPECTED_COMMAND may be given. If neither are
# given, 'var_to_set' is set to 1.
#
macro(opendaq_check_dependency)

    cmake_parse_arguments(OPENDAQ_CHECK_DEP
        ""
        "VAR;EXPECT_TARGET;EXPECT_COMMAND"
        ""
        ${ARGN}
    )

    # assume it's there
    set(${OPENDAQ_CHECK_DEP_VAR} 1)

    # it's not there if the expected target is not a target
    if (OPENDAQ_CHECK_DEP_EXPECT_TARGET AND NOT TARGET ${OPENDAQ_CHECK_DEP_EXPECT_TARGET})
        message(WARNING "Dependency does not provide expected target ${OPENDAQ_CHECK_DEP_EXPECT_TARGET}!")
        set(${OPENDAQ_CHECK_DEP_VAR} 0)
    endif()

    # it's not there if the expected command is not a command
    if (OPENDAQ_CHECK_DEP_EXPECT_COMMAND AND NOT COMMAND ${OPENDAQ_CHECK_DEP_EXPECT_COMMAND})
        message(WARNING "Dependency does not provide expected command ${OPENDAQ_CHECK_DEP_EXPECT_COMMAND}!")
        set(${OPENDAQ_CHECK_DEP_VAR} 0)
    endif()

endmacro()

#
# opendaq_dependency(
#     NAME                depname             # name for find_package()
#     REQUIRED_VERSION    1.2.3               # minimum version number for find_package() and/or pkgconfig
#     ...SourceOptions...
#     [ FIND_PACKAGE_NAME depName ]           # overrides the name used in find_package() (default is NAME)
#     [ LOG_LEVEL         WARNING ]           # sets CMAKE_MESSAGE_LOG_LEVEL when fetching
#     [ PKGCONFIG_NAME    nameInPkgconfig ]   # if set, try pkgconfig with this name if find_package() fails
#     [ SOURCE_SUBDIR     subdir ]            # add_subdirectory() this subdir in source instead of top level
#     [ ADD_FETCH_ALIAS   Alias=Target ... ]  # add specified aliases if dep was fetched (not config mode)
#     [ EXPECT_TARGET     depname::depname ]  # if set, asserts that the specified target is provided
#     [ EXPECT_COMMNAD    some_command ]      # if set, asserts that the specified command is provided
#     [ PATCH_FILES       ... ]               # if set, specified patches are applied before building
#     [ GIT_SHALLOW       OFF ]               # overrides the default GIT_SHALLOW mode (default is ON)
# )
#
# SourceOptions are:
#
#     For Git repositories:
#         GIT_REPOSITORY      https://github.com/depname/depname.git
#         GIT_REF             v1.2.4
#
#     For https resources:
#         URL                 https://dep.io/dep.tar.gz
#         [ URL_HASH          SHA256=... ]
#
# This macro sets:
#
#     depname_FOUND       # 1 if found with find_package() or pkgconfig, else 0
#     depname_FETCHED     # 1 if fetched with FetchContent, else 0
#
# This macro adds an external openDAQ dependency in a standardized way. A CMake option
# 'OPENDAQ_ALWAYS_FETCH_name' is defined allowing the user to specify whether to always fetch or
# whether to allow CMake to find a locally installed version. If the latter, this macro first
# attempts to locate a compatible version of the dependency. If unsuccessful, or if configured to
# always fetch, this macro uses FetchContent to fetch a specified version of the dependency. In
# either case, a sanity check is performed to ensure the expected targets and/or commands are
# available. At each step, useful status messages are printed for the user.
#
macro(opendaq_dependency)

    cmake_parse_arguments(OPENDAQ_DEP
        "OPTIONAL"
        "NAME;REQUIRED_VERSION;EXPECT_TARGET;EXPECT_COMMAND;GIT_REPOSITORY;GIT_REF;GIT_SHALLOW;URL;URL_HASH;FETCH_LOG_LEVEL;PKGCONFIG_NAME;FIND_PACKAGE_NAME;GIT_SUBMODULES;SOURCE_SUBDIR"
        "PATCH_FILES;ADD_FETCH_ALIAS"
        ${ARGN})

    string(TOLOWER "${OPENDAQ_DEP_NAME}" OPENDAQ_DEP_NAME_LOWER)
    string(TOUPPER "${OPENDAQ_DEP_NAME}" OPENDAQ_DEP_NAME_UPPER)

    set_cmake_folder_context(TARGET_FOLDER_NAME)

    # define an option for the user to specify if the dependency should always be
    # fetched; use OPENDAQ_ALWAYS_FETCH_DEPENDENCIES as the option's default value
    option(
        OPENDAQ_ALWAYS_FETCH_${OPENDAQ_DEP_NAME_UPPER}
        "Ignore any installed ${OPENDAQ_DEP_NAME} and always build from source"
        ${OPENDAQ_ALWAYS_FETCH_DEPENDENCIES})

    # initially it's neither found nor fetched
    set(${OPENDAQ_DEP_NAME}_FOUND OFF)
    set(${OPENDAQ_DEP_NAME}_FETCHED OFF)

    # default find_package() name to NAME
    if (NOT OPENDAQ_DEP_FIND_PACKAGE_NAME)
        set(OPENDAQ_DEP_FIND_PACKAGE_NAME ${OPENDAQ_DEP_NAME})
    endif()

    if (NOT OPENDAQ_ALWAYS_FETCH_${OPENDAQ_DEP_NAME_UPPER})

        find_package(${OPENDAQ_DEP_FIND_PACKAGE_NAME} ${OPENDAQ_DEP_REQUIRED_VERSION} QUIET GLOBAL)

        if (${OPENDAQ_DEP_FIND_PACKAGE_NAME}_FOUND)

            set(${OPENDAQ_DEP_NAME}_FOUND 1)
            message(STATUS "Found ${OPENDAQ_DEP_NAME}: ${${OPENDAQ_DEP_NAME}_VERSION} at ${${OPENDAQ_DEP_NAME}_CONFIG}")

        elseif (OPENDAQ_DEP_PKGCONFIG_NAME)

            find_package(PkgConfig)
            if (PKG_CONFIG_FOUND)
                pkg_check_modules(${OPENDAQ_DEP_NAME} ${OPENDAQ_DEP_PKGCONFIG_NAME}>=${OPENDAQ_DEP_REQUIRED_VERSION} IMPORTED_TARGET GLOBAL)
                if (${OPENDAQ_DEP_NAME}_FOUND)
                    add_library(${OPENDAQ_DEP_EXPECT_TARGET} ALIAS PkgConfig::${OPENDAQ_DEP_NAME})
                    message(STATUS "Found ${OPENDAQ_DEP_NAME} >= ${OPENDAQ_DEP_REQUIRED_VERSION} with pkgconfig")
                endif()
            endif()

        endif()

        if (${OPENDAQ_DEP_NAME}_FOUND)

            # make sure the thing found provides the expected targets/commands
            opendaq_check_dependency(
                VAR OPENDAQ_DEP_TARGETS_OK
                EXPECT_TARGET "${OPENDAQ_DEP_EXPECT_TARGET}"
                EXPECT_COMMAND "${OPENDAQ_DEP_EXPECT_COMMAND}"
            )

            if (NOT OPENDAQ_DEP_TARGETS_OK)
                message(FATAL_ERROR "Dependency ${OPENDAQ_DEP_NAME} does not provide expected targets/commands!")
            endif()

        endif()

    endif()

    # not found or user specified ..._ALWAYS_FETCH_...
    if (NOT ${OPENDAQ_DEP_NAME}_FOUND)

        message(STATUS "Fetching ${OPENDAQ_DEP_NAME} ${OPENDAQ_DEP_GIT_REF}...")

        get_custom_fetch_content_params(${OPENDAQ_DEP_NAME} FC_PARAMS)

        # add a patch command if any patches specified
        set(PATCH_PARAMS)
        if (OPENDAQ_DEP_PATCH_FILES)
            string(REPLACE ";" "|" PATCHES "${OPENDAQ_DEP_PATCH_FILES}")
            set(PATCH_PARAMS PATCH_COMMAND
                ${CMAKE_COMMAND}
                    -DPATCHES=${PATCHES}
                    -DREQUIRED_VERSION=${OPENDAQ_DEP_FETCH_VERSION}
                    -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
                    -P ${opendaq_dependency__internal_dir}/patch.cmake
                    ${PATCH_FILES})
        endif()

        # if LOG_LEVEL given, push it onto CMAKE_MESSAGE_LOG_LEVEL
        set(OPENDAQ_DEP_OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
        if (OPENDAQ_DEP_FETCH_LOG_LEVEL)
            set(CMAKE_MESSAGE_LOG_LEVEL ${OPENDAQ_DEP_FETCH_LOG_LEVEL})
        endif()

        set(PARAMS_GIT_SHALLOW ON)
        if (DEFINED OPENDAQ_DEP_GIT_SHALLOW)
            set(PARAMS_GIT_SHALLOW ${OPENDAQ_DEP_GIT_SHALLOW})
        endif()

        set(SOURCE_PARAMS)
        if (OPENDAQ_DEP_GIT_REPOSITORY)
            set(SOURCE_PARAMS
                GIT_REPOSITORY ${OPENDAQ_DEP_GIT_REPOSITORY}
                GIT_TAG ${OPENDAQ_DEP_GIT_REF}
                GIT_PROGRESS ON
                GIT_SHALLOW ${PARAMS_GIT_SHALLOW}
                GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
            )
        elseif (OPENDAQ_DEP_URL)
            set(SOURCE_PARAMS
                URL ${OPENDAQ_DEP_URL}
                URL_HASH "${OPENDAQ_DEP_URL_HASH}"
            )
        else()
            message(FATAL_ERROR "No valid source given for opendaq_dependency()!")
        endif()

        if (OPENDAQ_DEP_SOURCE_SUBDIR)
            list(APPEND SOURCE_PARAMS SOURCE_SUBDIR ${OPENDAQ_DEP_SOURCE_SUBDIR})
        endif()

        FetchContent_Declare(${OPENDAQ_DEP_NAME}
            ${SOURCE_PARAMS}
            ${OPENDAQ_DEP_GIT_SUBMODULES}
            ${PATCH_PARAMS}
            ${FC_PARAMS}
        )

        FetchContent_MakeAvailable(${OPENDAQ_DEP_NAME})

        # pop CMAKE_MESSAGE_LOG_LEVEL
        set(CMAKE_MESSAGE_LOG_LEVEL ${OPENDAQ_DEP_OLD_LOG_LEVEL})

        # in fetched (add_subdirectory) mode, some packages do not create target aliases
        # normally expected in config mode (find_package) as these are only in its *Config.cmake
        # files; we will add these aliases ourselves now if requested by the caller
        foreach(alias ${OPENDAQ_DEP_ADD_FETCH_ALIAS})
            if(alias MATCHES "^([^=]+)=([^=]+)$")
                set(alias_from "${CMAKE_MATCH_1}")
                set(alias_to "${CMAKE_MATCH_2}")
                add_library(${alias_from} ALIAS ${alias_to})
            else()
                message(FATAL_ERROR "opendaq_dependency ADD_FETCH_ALIAS must be of the form Alias=RealTarget")
            endif()
        endforeach()

        # make sure the thing fetched provides the expected targets/commands
        opendaq_check_dependency(
            VAR ${OPENDAQ_DEP_NAME}_FETCHED
            EXPECT_TARGET "${OPENDAQ_DEP_EXPECT_TARGET}"
            EXPECT_COMMAND "${OPENDAQ_DEP_EXPECT_COMMAND}"
        )

        if (NOT OPENDAQ_DEP_OPTIONAL AND NOT ${OPENDAQ_DEP_NAME}_FETCHED)
            message(FATAL_ERROR "Fetched dependency ${OPENDAQ_DEP_NAME} does not provide expected targets/commands!")
        endif()
    endif()

endmacro()
