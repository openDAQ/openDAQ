# Generates bindings and wrappers for the specified RT Core interface header.
#
# FILENAME
#    - Input C++ header interface file name.
#    - Will be pefixed by RTGEN_OUTPUT_DIR.
#
# OUTFILE
#    - The C++ SmartPointer file name.
#    - Will be prefixed by RTGEN_HEADERS_DIR.
#
# OUT_VAR
#    - Output variable name containing full paths to the C++ header and SmartPtr
#
# LANGUAGES
#   - ALL
#   - ONLY LANG [LANG...]
#
#---------------------------------------------------------------------------------
# Configuration variables:
#
# RTGEN_OUTPUT_DIR
#   - Where to output the generated binding files.
#
# RTGEN_OUTPUT_SRC_DIR
#    - Where to output the generated binding source files.
#
# RTGEN_HEADERS_DIR
#   - Where to look for interface header files.
#
# RTGEN_VERBOSE
#   - Output additional log messages and more extensive error reports.
#
# RTGEN_NO_TIMESTAMP
#   - Date & time in generated files replaced by D-E-B-U-G.
#
#---------------------------------------------------------------------------------
function(_rtgen_interface FILENAME OUTFILES_VAR)
    if (NOT DEFINED RTGEN_OUTPUT_DIR)
        message(FATAL_ERROR "RTGen Output directory is not defined!")
    endif()

    if (NOT DEFINED RTGEN_HEADERS_DIR)
        message(FATAL_ERROR "RTGen Headers directory is not defined!")
    endif()

    if (NOT DEFINED RTGEN_LIBRARY_NAME)
        set(RTGEN_LIBRARY_NAME ${PROJECT_NAME})
    endif()

    set(LANGUAGES Cpp ${OPENDAQ_GENERATE_BINDINGS_LANG})

    # Expand the bindings dir
    get_filename_component(CURR_BINDINGS_DIR
                           "${CMAKE_CURRENT_BINARY_DIR}/../bindings"
                           ABSOLUTE)

    # Expand the output dir
    get_filename_component(RTGEN_OUTPUT_DIR
                           "${RTGEN_OUTPUT_DIR}"
                           ABSOLUTE)

    # Expand the headers dir
    get_filename_component(RTGEN_HEADERS_DIR
                           "${RTGEN_HEADERS_DIR}"
                           ABSOLUTE)

    #set(RTGEN_OPTIONS_GLOBAL)
    set(_FILENAME ${FILENAME})
    set(FILENAME "${RTGEN_HEADERS_DIR}/${FILENAME}")

    opendaq_create_dir(${CURR_BINDINGS_DIR})

    if(RTGEN_VERBOSE)
        list(APPEND RTGEN_OPTIONS_GLOBAL -v)
    endif()

    if(RTGEN_NO_TIMESTAMP)
        list(APPEND RTGEN_OPTIONS_GLOBAL -nt)
    endif()

    if (LANGUAGES STREQUAL "ALL")
        set(ARGN cpp delphi python)
    else()
        set(ARGN ${LANGUAGES})
    endif()

    foreach(LANG ${ARGN})
        set(RTGEN_OPTIONS ${RTGEN_OPTIONS_GLOBAL})

        string(TOLOWER "${LANG}" LOWERCASE_LANG)

        if (LOWERCASE_LANG STREQUAL "cpp")
            # If geneating Cpp add SmartPtr switch
            # Set the output directory to generated headers dir (The output directory must exist).
            list(APPEND RTGEN_OPTIONS -p)
        elseif (LOWERCASE_LANG STREQUAL "delphi")
            list(APPEND RTGEN_OPTIONS -cns OpenDAQ)
        endif()

        if (NOT LOWERCASE_LANG STREQUAL "cpp")
            # If generating bindings set the output directory to "bindings/{LANG}"
            # Create the directory if it doesn't exist yet
            if (LOWERCASE_LANG STREQUAL "csharp")
                set_output_dir_for_bindings(RTGEN_OUTPUT_DIR)
            else()
                set(RTGEN_OUTPUT_DIR "${CURR_BINDINGS_DIR}/${LANG}")
            endif()
            if (NOT EXISTS RTGEN_OUTPUT_DIR)
                opendaq_create_dir(${RTGEN_OUTPUT_DIR})
            endif()
        endif()

        if (RTGEN_NAMESPACE)
            if (LOWERCASE_LANG STREQUAL "delphi" AND RTGEN_NAMESPACE STREQUAL "${SDK_TARGET_NAMESPACE}")
                list(APPEND RTGEN_OPTIONS -ns "OpenDAQ")
            else()
                list(APPEND RTGEN_OPTIONS -ns ${RTGEN_NAMESPACE})
            endif()
        endif()

        if (LOWERCASE_LANG STREQUAL "csharp")
            modify_or_set_namespace(RTGEN_OPTIONS RTGEN_NAMESPACE)
        endif()

        set(_LANG_COMMAND ${MONO_C} ${RTGEN}
                                    ${RTGEN_OPTIONS}
                                    -ln ${RTGEN_LIBRARY_NAME}
                                    -d "\"${RTGEN_OUTPUT_DIR}\""
                                    -f "\"${RTGEN_OUTPUT_SRC_DIR}\""
                                    --lang ${LOWERCASE_LANG}
                                    --source="${FILENAME}" &&)
        set(RTGEN_COMMAND ${RTGEN_COMMAND} ${_LANG_COMMAND})

        if (DEFINED RTGEN_VERBOSE)
            message(STATUS "Adding RTGen command:")
            message(STATUS "${_LANG_COMMAND}")
        endif()
    endforeach()

    # Remove the trailing "&&"
    string(LENGTH "${RTGEN_COMMAND}" _COMMAND_LENGTH)
    math(EXPR _COMMAND_LENGTH "${_COMMAND_LENGTH}-2")
    string(SUBSTRING "${RTGEN_COMMAND}" 0 ${_COMMAND_LENGTH} RTGEN_COMMAND)

    add_custom_command(
        OUTPUT ${${OUTFILES_VAR}}
        COMMAND ${RTGEN_COMMAND}
        MAIN_DEPENDENCY ${FILENAME}
        DEPENDS ${RTGEN} ${RTGEN_DEPENDENT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating bindings for: ${_FILENAME}"
        #VERBATIM
    )
endfunction(_rtgen_interface)

function(opendaq_set_generated FILE TYPE)
    set_source_files_properties(${FILE} PROPERTIES GENERATED TRUE)

    if (TYPE STREQUAL "HEADER")
        source_group("Generated\\Header Files" FILES ${FILE})
    elseif(TYPE STREQUAL "SOURCE")
        source_group("Generated\\Source Files" FILES ${FILE})
    else()
        message(FATAL_ERROR "Unknown file type \"${TYPE}\" when setting file \"${FILE}\" to generated.")
        return()
    endif()

    set_source_files_properties(${FILE} PROPERTIES GENERATED TRUE)
endfunction(opendaq_set_generated)

#-------------------------------------------------------------------------------------
# Returns the full file path to the generated file based on the interface header name
# and the type of the file generated
#
# OUT_VAR_BASE
#    - The variable base to which to append the full path
#    - Eg.: SRC_Channel produces
#        - SRC_Channel_PublicHeaders
#        - SRC_Channel_PrivateHeaders
#        - SRC_Channel_Cpp
#
# TYPE
#    - The type of the file generated
#    - One of:
#        - DECORATOR
#        - PROPERTY_CLASS
#
# BASE_NAME
#    - The base file name of the interface header
#    - Eg.: "channel" produces
#        - channel_decorator.h
#        - channel_property_class.h
#        - channel_property_class.cpp
#-------------------------------------------------------------------------------------
function(rtgen_add_file OUT_VAR_BASE TYPE BASE_NAME)

    if (TYPE STREQUAL "DECORATOR")
        set(OUT "${OUT_VAR_BASE}_PublicHeaders")
        set(FILE_H "${RTGEN_OUTPUT_DIR}/${BASE_NAME}_decorator.h")
        list(APPEND ${OUT} ${FILE_H})

        opendaq_set_generated(${FILE_H} HEADER)
    elseif(${TYPE} STREQUAL "PROPERTY_CLASS")
        set(OUT "${OUT_VAR_BASE}_PublicHeaders")
        set(FILE_H "${RTGEN_OUTPUT_DIR}/${BASE_NAME}_property_class.h")
        list(APPEND ${OUT} ${FILE_H})

        set(${OUT_VAR_BASE}_Cpp ${${OUT_VAR_BASE}_Cpp} PARENT_SCOPE)

        opendaq_set_generated(${FILE_H} HEADER)
    else()
        message(FATAL_ERROR "Unknown RTGen type \"${TYPE}\"")
        return()
    endif()

    set(${OUT} ${${OUT}} PARENT_SCOPE)
endfunction(rtgen_add_file)

#-------------------------------------------------------------------------------------
# Generates smart-ptr, decorator and property-class from an interface header file.
#
# FILENAME
#    - The interface header file to generate from (e.g.: channel.h).
#
# OUT_VAR_BASE
#    - A list variable where all full paths to the generated files will get stored
#    - Additionally a few more variables will get defined
#        - ${OUT_VAR_BASE}_PublicHeaders  (smart-ptr, decorator, interface-file, property-class header)
#        - ${OUT_VAR_BASE}_PrivateHeaders ([none])
#        - ${OUT_VAR_BASE}_Cpp (property-class source)
#
# ARGN
#    - Types of files to generate (SmartPtr is always generated as ${FILENAME}_ptr.h)
#       - DECORATOR
#           - ${FILENAME}_decorator.h
#       - PROPERTY_CLASS
#           - ${FILENAME}_property_class.h
#
#-------------------------------------------------------------------------------------
function(rtgen OUT_VAR_BASE FILENAME)
    set(RTGEN_LANGS Cpp ${OPENDAQ_GENERATE_BINDINGS_LANG})

    # Get the filename without extension
    get_filename_component(RTGEN_BASE_NAME "${FILENAME}" NAME_WE)

    # SmartPtr is always generated so add it to the list
    set(SMARTPTR_FILE "${RTGEN_OUTPUT_DIR}/${RTGEN_BASE_NAME}_ptr.h")
    opendaq_set_generated(${SMARTPTR_FILE} HEADER)

    set(${OUT_VAR_BASE}_PublicHeaders ${SMARTPTR_FILE})

    foreach (TYPE ${ARGN})
        rtgen_add_file(${OUT_VAR_BASE} ${TYPE} ${RTGEN_BASE_NAME})
    endforeach()

    # Setting it in parent scope does not define it in current scope
    set(${OUT_VAR_BASE} ${${OUT_VAR_BASE}_Cpp} ${${OUT_VAR_BASE}_PublicHeaders} ${${OUT_VAR_BASE}_PrivateHeaders})

    _rtgen_interface(${FILENAME} ${OUT_VAR_BASE})

    list(APPEND ${OUT_VAR_BASE}_PublicHeaders ${RTGEN_HEADERS_DIR}/${FILENAME})
    list(APPEND ${OUT_VAR_BASE} ${RTGEN_HEADERS_DIR}/${FILENAME})

    set(${OUT_VAR_BASE} ${${OUT_VAR_BASE}} PARENT_SCOPE)
    set(${OUT_VAR_BASE}_Cpp ${${OUT_VAR_BASE}_Cpp} PARENT_SCOPE)
    set(${OUT_VAR_BASE}_PublicHeaders ${${OUT_VAR_BASE}_PublicHeaders} PARENT_SCOPE)
    set(${OUT_VAR_BASE}_PrivateHeaders ${${OUT_VAR_BASE}_PrivateHeaders} PARENT_SCOPE)
endfunction(rtgen)

function(rtgen_t OUT_VAR_BASE FILENAME SAMPLE_TYPES_LIST)
    list(GET ${SAMPLE_TYPES_LIST} 0 SAMPLE_TYPES_JSON)
    list(GET ${SAMPLE_TYPES_LIST} 1 SAMPLE_TYPES_SOURCE)

    set(_OPTIONS ${RTGEN_OPTIONS_GLOBAL})
    set(_DEPENDS ${RTGEN_DEPENDENT_SOURCES})

    list(APPEND RTGEN_DEPENDENT_SOURCES ${SAMPLE_TYPES_SOURCE} ${SAMPLE_TYPES_JSON})
    list(APPEND RTGEN_OPTIONS_GLOBAL -m "\"${SAMPLE_TYPES_JSON}\"")

    rtgen(${OUT_VAR_BASE} ${FILENAME} ${ARGN})
    set(${OUT_VAR_BASE} ${${OUT_VAR_BASE}} PARENT_SCOPE)
    set(${OUT_VAR_BASE}_Cpp ${${OUT_VAR_BASE}_Cpp} PARENT_SCOPE)
    set(${OUT_VAR_BASE}_PublicHeaders ${${OUT_VAR_BASE}_PublicHeaders} PARENT_SCOPE)
    set(${OUT_VAR_BASE}_PrivateHeaders ${${OUT_VAR_BASE}_PrivateHeaders} PARENT_SCOPE)

    set(RTGEN_OPTIONS_GLOBAL ${_OPTIONS})
    set(RTGEN_DEPENDENT_SOURCES ${_DEPENDS})
endfunction(rtgen_t)

function(rtgen_config LIB_NAME LIB_OUTPUT_NAME MAJOR_VER MINOR_VER PATCH_VER)
    if (${ARGC} EQUAL 6)
        set(_NAMESPACE_NAME ${ARGV5})
    else()
        set(_NAMESPACE_NAME "${SDK_TARGET_NAMESPACE}")
    endif()

    # Expand the bindings dir
    get_filename_component(CURR_BINDINGS_DIR
                           "${CMAKE_CURRENT_BINARY_DIR}/../bindings"
                           ABSOLUTE
    )

    set(RTGEN_OPTIONS_GLOBAL)
    opendaq_create_dir(${CURR_BINDINGS_DIR})

    if(RTGEN_VERBOSE)
        list(APPEND RTGEN_OPTIONS_GLOBAL -v)
    endif()

    if(RTGEN_NO_TIMESTAMP)
        list(APPEND RTGEN_OPTIONS_GLOBAL -nt)
    endif()

    set(LANGUAGES ${OPENDAQ_GENERATE_BINDINGS_LANG})

    foreach(LANG ${LANGUAGES})
        set(RTGEN_OPTIONS ${RTGEN_OPTIONS_GLOBAL})

        string(TOLOWER "${LANG}" LOWERCASE_LANG)
        if (NOT LOWERCASE_LANG STREQUAL "cpp")
            # If generating bindings set the output directory to "bindings/{LANG}"
            # Create the directory if it doesn't exist yet
            if (LOWERCASE_LANG STREQUAL "csharp")
                set_output_dir_for_bindings(RTGEN_OUTPUT_DIR)
            else()
                set(RTGEN_OUTPUT_DIR "${CURR_BINDINGS_DIR}/${LANG}")
            endif()
            if (NOT EXISTS RTGEN_OUTPUT_DIR)
                opendaq_create_dir(${RTGEN_OUTPUT_DIR})
            endif()
        endif()

        if (LOWERCASE_LANG STREQUAL "delphi" AND _NAMESPACE_NAME STREQUAL "${SDK_TARGET_NAMESPACE}")
            list(APPEND RTGEN_OPTIONS -ns "OpenDAQ")
        else()
            list(APPEND RTGEN_OPTIONS -ns "${_NAMESPACE_NAME}")
        endif()

        if (LOWERCASE_LANG STREQUAL "csharp")
            modify_or_set_namespace(RTGEN_OPTIONS RTGEN_NAMESPACE)
        endif()

        list(APPEND RTGEN_OPTIONS -ln ${LIB_NAME} -config -lo ${LIB_OUTPUT_NAME} -lv ${MAJOR_VER}.${MINOR_VER}.${PATCH_VER} -lang ${LOWERCASE_LANG})
        set(RTGEN_COMMAND ${MONO_C} ${RTGEN} ${RTGEN_OPTIONS} -d "${RTGEN_OUTPUT_DIR}")

        if (DEFINED RTGEN_VERBOSE)
            message(STATUS "Adding RTGen config command:")
            message(STATUS "${RTGEN_COMMAND}")
        endif()

        add_custom_command(
            TARGET ${LIB_OUTPUT_NAME} POST_BUILD
            DEPENDS ${RTGEN} ${CMAKE_CURRENT_LIST_FILE}
            COMMAND ${RTGEN_COMMAND}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Generating config file for: ${LIB_NAME}"
            VERBATIM
        )
    endforeach()
endfunction(rtgen_config)

function(rtgen_sample_types HEADER_NAME OUT_FILE)
    # Expand the headers dir
    get_filename_component(RTGEN_HEADERS_DIR
                           "${RTGEN_HEADERS_DIR}"
                           ABSOLUTE)

    set(FILENAME "${RTGEN_HEADERS_DIR}/${HEADER_NAME}")

    if (NOT EXISTS ${FILENAME})
        message(FATAL_ERROR "RTGen: predefined template sample types input header must exists. Called with: \"${HEADER_NAME}\" that resolved to: \"${FILENAME}\"")
    endif()

    if (NOT DEFINED RTGEN_LIBRARY_NAME)
        set(RTGEN_LIBRARY_NAME ${PROJECT_NAME})
    endif()

    set(RTGEN_OPTIONS ${RTGEN_OPTIONS_GLOBAL})
    set(RTGEN_COMMAND ${MONO_C} ${RTGEN}
                                ${RTGEN_OPTIONS}
                                -p
                                -ln ${RTGEN_LIBRARY_NAME}
                                -d "\"${RTGEN_OUTPUT_DIR}\""
                                -f "\"${RTGEN_OUTPUT_SRC_DIR}\""
                                -inlang corestructure_templates
                                -lang corestructure_templates
                                --source="${FILENAME}"
                                )

    get_filename_component(RTGEN_SAMPLE_TYPES_FILENAME "${FILENAME}" NAME_WE)
    set(SAMPLE_TYPES_JSON ${RTGEN_OUTPUT_DIR}/${RTGEN_SAMPLE_TYPES_FILENAME}.json)

    set(${OUT_FILE} ${SAMPLE_TYPES_JSON}
                    ${FILENAME}
                    PARENT_SCOPE)

    add_custom_command(OUTPUT ${SAMPLE_TYPES_JSON}
                       COMMAND ${RTGEN_COMMAND}
                       MAIN_DEPENDENCY ${FILENAME}
                       DEPENDS ${RTGEN}
                       WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                       COMMENT "Parsing pre-defined sample-type macros in ${HEADER_NAME}"
    )
endfunction(rtgen_sample_types)

function(get_rel_project_dir OUT_REL_PROJECT_DIR)
    # Get the relative build project path
    string(LENGTH "${CMAKE_BINARY_DIR}" _PATH_LENGTH)
    math(EXPR _PATH_LENGTH "${_PATH_LENGTH}+1") #include trailing forward slash
    string(SUBSTRING "${CMAKE_CURRENT_BINARY_DIR}" ${_PATH_LENGTH} -1 _REL_PROJECT_DIR)

    # Remove last sub-dir
    string(FIND "${_REL_PROJECT_DIR}" "/" _PATH_LENGTH REVERSE)
    string(SUBSTRING "${_REL_PROJECT_DIR}" 0 ${_PATH_LENGTH} _REL_PROJECT_DIR)

    # Set output argument
    set(${OUT_REL_PROJECT_DIR} ${_REL_PROJECT_DIR} PARENT_SCOPE)
endfunction(get_rel_project_dir)

function(set_output_dir_for_bindings OUT_OUTPUT_DIR)
    get_rel_project_dir(_REL_PROJECT_DIR)

    # Expand the new output dir
    get_filename_component(_OUTPUT_DIR
                           "${CMAKE_SOURCE_DIR}/build/bindings/${LANG}/${_REL_PROJECT_DIR}"
                           ABSOLUTE)

    #flat output
    #set(_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build/bindings/${LANG}")

    # Set output argument
    set(${OUT_OUTPUT_DIR} ${_OUTPUT_DIR} PARENT_SCOPE)
endfunction(set_output_dir_for_bindings)

function(modify_or_set_namespace OUT_OPTIONS NAMESPACE_NAME)
    get_rel_project_dir(_REL_PROJECT_DIR)

    set(_OPTIONS ${${OUT_OPTIONS}})

    # Modify/add the namespace option to "<base-ns>.<rel-project-structure>"
    string(REPLACE "/" "." _SUB_NS "${_REL_PROJECT_DIR}")
    if (${NAMESPACE_NAME})
        # Modify
        list(FIND _OPTIONS -ns _NS_INDEX)
        math(EXPR _NS_INDEX "${_NS_INDEX}+1") #index of namespace value
        list(REMOVE_AT _OPTIONS "${_NS_INDEX}")
        list(INSERT _OPTIONS "${_NS_INDEX}" "${${NAMESPACE_NAME}}.${_SUB_NS}")
    else()
        # Add
        list(APPEND _OPTIONS -ns "${_SUB_NS}")
    endif()

    # Set output argument
    set(${OUT_OPTIONS} ${_OPTIONS} PARENT_SCOPE)
endfunction(modify_or_set_namespace)
