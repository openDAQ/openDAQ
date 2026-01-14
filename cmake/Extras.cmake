function(print_var VARIABLE_NAME) # not used
    message(STATUS "${VARIABLE_NAME}: ${${VARIABLE_NAME}}")
endfunction()

function(get_current_folder OUTFOLDER) # not used
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

function(check_if_files_exist DIR INPUT_FILES) # not used
    foreach (INPUT_FILE ${${INPUT_FILES}})
        set(CUR_FILE ${DIR}/${INPUT_FILE})
        if (NOT EXISTS ${CUR_FILE})
            message(FATAL_ERROR "${CUR_FILE} does not exist")
        endif()
    endforeach()
endfunction(check_if_files_exist)

function(dump_cmake_variables) # is not used at all
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

macro(is_git_repository_root DIRECTORY_PATH FUNC_RESULT) # is not used at all
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
