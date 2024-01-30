set(SDK_LIBRARY_VARIANTS
    default
    dev
    CACHE INTERNAL ""
)

function(opendaq_add_library LIB_BASE_NAME)
    foreach(VARIANT_TYPE ${SDK_LIBRARY_VARIANTS})
        set(LIB_ARGS ${ARGN})

        if ("${VARIANT_TYPE}" STREQUAL "default")
            set(VARIANT_SUFFIX "")
        elseif("${VARIANT_TYPE}" STREQUAL "dev")
           set(VARIANT_SUFFIX "_${VARIANT_TYPE}")

           # convert to object library
           list(POP_FRONT LIB_ARGS)
           list(INSERT LIB_ARGS 0 OBJECT)
        else()
            set(VARIANT_SUFFIX "_${VARIANT_TYPE}")
        endif()

        set(VARIANT_TARGET ${SDK_TARGET_NAMESPACE}_${LIB_BASE_NAME}${VARIANT_SUFFIX})
        set(VARIANT_ALIAS ${SDK_TARGET_NAMESPACE}::${LIB_BASE_NAME}${VARIANT_SUFFIX})

        add_library(${VARIANT_TARGET} ${LIB_ARGS})
        add_library(${VARIANT_ALIAS} ALIAS ${VARIANT_TARGET})

        set_target_properties(${VARIANT_TARGET}
            PROPERTIES
                PUBLIC_HEADER "${SRC_PublicHeaders}"
                OPENDAQ_COMPONENT TRUE
                OPENDAQ_TARGET_VARIANT ${VARIANT_TYPE}
        )

        if (MSVC)
            target_compile_options(${VARIANT_TARGET} PRIVATE /wd4100)
        endif()

        if ("${VARIANT_TYPE}" STREQUAL "default")
            target_compile_definitions(${VARIANT_TARGET}
                PRIVATE
                    BUILDING_SHARED_LIBRARY
            )

            create_version_header(${VARIANT_TARGET}
                                  INCLUDE_FOLDER ${MAIN_TARGET}
                                  NO_RC
            )
        elseif ("${VARIANT_TYPE}" STREQUAL "dev")
            target_compile_definitions(${VARIANT_TARGET}
                PRIVATE
                    BUILDING_STATIC_LIBRARY
                    OPENDAQ_SKIP_DLL_IMPORT
            )

            #add_dependencies(${VARIANT_TARGET} ${SDK_TARGET_NAMESPACE}::${LIB_BASE_NAME})
        endif()

        opendaq_set_output_lib_name(${VARIANT_TARGET} ${PROJECT_VERSION_MAJOR})

        install(TARGETS ${VARIANT_TARGET}
            EXPORT ${SDK_NAME}
            RUNTIME
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                COMPONENT ${SDK_NAME}_${MAIN_TARGET}_Runtime
            LIBRARY
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT          ${SDK_NAME}_${MAIN_TARGET}_Runtime
                NAMELINK_COMPONENT ${SDK_NAME}_${MAIN_TARGET}_Development
            ARCHIVE
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT ${SDK_NAME}_${MAIN_TARGET}_Development
            PUBLIC_HEADER
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MAIN_TARGET}
                COMPONENT ${SDK_NAME}_${MAIN_TARGET}_Development
        )
    endforeach()

    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bindings/"
            DESTINATION bindings
    )
endfunction()

function(opendaq_component_target_variant COMPONENT_NAME VARIANT_TYPE VARIANT_TARGET)
    if (VARIANT_TYPE STREQUAL "default")
        set(VARIANT_SUFFIX "")
    else()
        set(VARIANT_SUFFIX "_${VARIANT_TYPE}")
    endif()

    set(${VARIANT_TARGET} ${SDK_TARGET_NAMESPACE}_${COMPONENT_NAME}${VARIANT_SUFFIX} PARENT_SCOPE)
endfunction()


function(opendaq_target_link_libraries)
    # opendaq_call_command("target_link_libraries" ${ARGV})

    set(LIB_BASE_NAME ${ARGV0})
    set(COMMAND_ARGS ${ARGV})
    list(POP_FRONT COMMAND_ARGS)

    foreach(VARIANT_TYPE ${SDK_LIBRARY_VARIANTS})
        opendaq_component_target_variant(${LIB_BASE_NAME} ${VARIANT_TYPE} VARIANT_TARGET)

        if ("${VARIANT_TYPE}" STREQUAL "default")
            target_link_libraries(${VARIANT_TARGET} ${COMMAND_ARGS})

            message("${VARIANT_TARGET}: ${COMMAND_ARGS}")
        else()
            set(VARIANT_ARGS )
            set(VARIANT_SUFFIX "_${VARIANT_TYPE}")

            foreach(COMMAND_ARG ${COMMAND_ARGS})
                if (NOT COMMAND_ARG STREQUAL "${SDK_TARGET_NAMESPACE}::coretypes"   AND
                    NOT COMMAND_ARG STREQUAL "${SDK_TARGET_NAMESPACE}::coreobjects" AND
                    COMMAND_ARG MATCHES "^${SDK_TARGET_NAMESPACE}::"
                )
                    list(APPEND VARIANT_ARGS "${COMMAND_ARG}${VARIANT_SUFFIX}")
                    continue()
                endif()

                #if (TARGET ${COMMAND_ARG})
                #    get_target_property(IS_TARGET_DAQ_COMPONENT ${COMMAND_ARG} OPENDAQ_COMPONENT)
                #    if (IS_TARGET_DAQ_COMPONENT)
                #        list(APPEND VARIANT_ARGS "${COMMAND_ARG}${VARIANT_SUFFIX}")
                #        continue()
                #    endif()
                #endif()

                list(APPEND VARIANT_ARGS ${COMMAND_ARG})
            endforeach()

            target_link_libraries(${VARIANT_TARGET} ${VARIANT_ARGS})

            message("${VARIANT_TARGET}: ${VARIANT_ARGS}")
        endif()
    endforeach()
endfunction()

function(opendaq_call_command COMMAND_NAME)
    set(COMMAND_ARGS ${ARGN})
    list(POP_FRONT COMMAND_ARGS)

    foreach(VARIANT_TYPE ${SDK_LIBRARY_VARIANTS})
        if ("${VARIANT_TYPE}" STREQUAL "default")
            set(VARIANT_SUFFIX "")
        else()
            set(VARIANT_SUFFIX "_${VARIANT_TYPE}")
        endif()

        set(LIB_BASE_NAME ${ARGV1})
        set(VARIANT_TARGET ${SDK_TARGET_NAMESPACE}_${LIB_BASE_NAME}${VARIANT_SUFFIX})

        cmake_language(CALL ${COMMAND_NAME} ${VARIANT_TARGET} ${COMMAND_ARGS})
    endforeach()
endfunction()

function(opendaq_target_include_directories)
    opendaq_call_command("target_include_directories" ${ARGV})
endfunction()

function(opendaq_set_target_properties)
    opendaq_call_command("set_target_properties" ${ARGV})
endfunction()

function(opendaq_target_compile_options)
    opendaq_call_command("target_compile_options" ${ARGV})
endfunction()

function(opendaq_target_compile_definitions)
    opendaq_call_command("target_compile_definitions" ${ARGV})
endfunction()

function(opendaq_add_dependencies)
    opendaq_call_command("add_dependencies" ${ARGV})
endfunction()
