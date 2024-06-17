function(opendaq_create_version_header LIB_NAME OUTPUT_DIR HEADER_PREFIX GENERATE_RC GENERATE_HEADER)
    set(TEMPLATE_DIR ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/version)

    if (GENERATE_HEADER)
        set(VERSION_HEADER ${OUTPUT_DIR}/${HEADER_PREFIX}version.h)

        string(TOUPPER ${LIB_NAME} UPPERCASE_LIB_NAME)
        configure_file(${TEMPLATE_DIR}/version.h.in ${VERSION_HEADER})
    endif()

    if (WIN32 AND GENERATE_RC)
        set(VERSION_RC ${OUTPUT_DIR}/version.rc)

        get_target_property(TARGET_SUFFIX ${LIB_NAME} SUFFIX)
        get_target_property(ORIGINAL_OUTPUT_NAME ${LIB_NAME} OUTPUT_NAME)

        if (TARGET_SUFFIX)
            set(LIB_TARGET_TYPE ${TARGET_SUFFIX})
        else()
            get_target_property(TARGET_TYPE ${LIB_NAME} TYPE)
            if (TARGET_TYPE STREQUAL "EXECUTABLE")
                set(LIB_TARGET_TYPE ${CMAKE_EXECUTABLE_SUFFIX})
            elseif (TARGET_TYPE STREQUAL "STATIC_LIBRARY")
                set(LIB_TARGET_TYPE ${CMAKE_STATIC_LIBRARY_SUFFIX})
            else()
                set(LIB_TARGET_TYPE ${CMAKE_SHARED_LIBRARY_SUFFIX})
            endif()
        endif()

        configure_file(${TEMPLATE_DIR}/version.rc.in ${VERSION_RC})
    endif()

    target_sources(${LIB_NAME} PRIVATE ${VERSION_HEADER} ${VERSION_RC})
endfunction()

function(opendaq_prepare_internal_runner TEST_TARGET)
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

    set(TEST_RUNNER test_${RUNNER_FOR}_internal)
    add_executable(${TEST_RUNNER} ${RUNNER_SOURCES})

    set_target_properties(${TEST_RUNNER} PROPERTIES DEBUG_POSTFIX _debug)

    target_link_libraries(${TEST_RUNNER} PRIVATE ${SDK_TARGET_NAMESPACE}::${MAIN_TARGET}_dev
                                                 ${SDK_TARGET_NAMESPACE}::test_utils
    )

    target_compile_definitions(${TEST_RUNNER} PRIVATE BUILDING_SHARED_LIBRARY)
	
    target_include_directories(
       ${TEST_RUNNER}
       INTERFACE $<TARGET_PROPERTY:${SDK_TARGET_NAMESPACE}::${MAIN_TARGET},INCLUDE_DIRECTORIES>
       INTERFACE $<TARGET_PROPERTY:${SDK_TARGET_NAMESPACE}::${MAIN_TARGET},INTERFACE_INCLUDE_DIRECTORIES>
    )

    if (MSVC)
        target_compile_options(${TEST_RUNNER} PRIVATE /wd4100)
    endif()

    set(${TEST_TARGET} ${TEST_RUNNER} PARENT_SCOPE)
endfunction()

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
