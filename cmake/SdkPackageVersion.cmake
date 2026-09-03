# Writes the header defining OPENDAQ_PACKAGE_VERSION: the package version with the revision hash
# appended, except on release branches. Runs at build time and only rewrites the header when the
# content changes, so a new revision rebuilds just the sources that include it.
#
# Inputs: PACKAGE_VERSION, SOURCE_DIR, TEMPLATE, OUTPUT; optional GIT_EXECUTABLE and BRANCH_OVERRIDE
# (the CI branch ref, used because CI builds check out a detached HEAD).

set(OPENDAQ_PACKAGE_VERSION_FULL "${PACKAGE_VERSION}")

if (GIT_EXECUTABLE)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short --verify -q HEAD
                    WORKING_DIRECTORY "${SOURCE_DIR}"
                    RESULT_VARIABLE GIT_RESULT
                    OUTPUT_VARIABLE REVISION_HASH
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (GIT_RESULT EQUAL 0 AND REVISION_HASH)
        if (BRANCH_OVERRIDE)
            string(REPLACE "refs/heads/" "" BRANCH_NAME "${BRANCH_OVERRIDE}")
        else()
            execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse -q --abbrev-ref HEAD
                            WORKING_DIRECTORY "${SOURCE_DIR}"
                            OUTPUT_VARIABLE BRANCH_NAME
                            ERROR_QUIET
                            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
        endif()

        if (NOT BRANCH_NAME MATCHES "^release/[0-9]+\\.[0-9]+(\\.[0-9])?(\\.[0-9])?(-[a-zA-Z0-9]*)?$")
            set(OPENDAQ_PACKAGE_VERSION_FULL "${PACKAGE_VERSION}_${REVISION_HASH}")
        endif()
    endif()
endif()

configure_file("${TEMPLATE}" "${OUTPUT}" @ONLY)
