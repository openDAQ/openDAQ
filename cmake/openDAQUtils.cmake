function (opendaq_set_module_properties MODULE_NAME LIB_MAJOR_VERSION)
    set(options "SKIP_INSTALL")
    set(oneValueArgs "")
    set(multiValueArgs "")
    cmake_parse_arguments(OPENDAQ_SET_MODULE_PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if (NOT DEFINED OPENDAQ_MODULE_SUFFIX)
		set(OPENDAQ_MODULE_SUFFIX ".module${CMAKE_SHARED_LIBRARY_SUFFIX}")
	endif()
	
    set_target_properties(${MODULE_NAME} PROPERTIES SUFFIX ${OPENDAQ_MODULE_SUFFIX})
    target_compile_definitions(${MODULE_NAME} PRIVATE BUILDING_SHARED_LIBRARY
                                                      OPENDAQ_TRACK_SHARED_LIB_OBJECT_COUNT
                                                      OPENDAQ_MODULE_EXPORTS
    )
    opendaq_set_output_lib_name(${MODULE_NAME} ${LIB_MAJOR_VERSION})

    if (NOT ${OPENDAQ_SET_MODULE_PARAMS_SKIP_INSTALL})
        install(TARGETS ${MODULE_NAME}
                EXPORT ${SDK_NAME}
                RUNTIME
                    DESTINATION ${CMAKE_INSTALL_BINDIR}/modules
                    COMPONENT ${SDK_NAME}_${MODULE_NAME}_Runtime
                LIBRARY
                    DESTINATION ${CMAKE_INSTALL_LIBDIR}/modules
                    COMPONENT          ${SDK_NAME}_${MODULE_NAME}_Runtime
                    NAMELINK_COMPONENT ${SDK_NAME}_${MODULE_NAME}_Development
                ARCHIVE
                    DESTINATION ${CMAKE_INSTALL_LIBDIR}/modules
                    COMPONENT ${SDK_NAME}_${MODULE_NAME}_Development
                PUBLIC_HEADER
                    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MODULE_NAME}
                    COMPONENT ${SDK_NAME}_${MODULE_NAME}_Development
        )
    endif()
endfunction()

function(opendaq_is_64bit_build ARGS)
    set(BUILD_64Bit Off)

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
        set(BUILD_64Bit On)
    endif()

    if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64$")  # arm architecture 64bit
        set(BUILD_64Bit On)
    endif()

    set(${ARGS} ${BUILD_64Bit} PARENT_SCOPE)
endfunction()

function(opendaq_set_output_lib_name LIB_NAME MAJOR_VERSION)
    set(TEMP_NAME ${LIB_NAME})

    opendaq_is_64bit_build(BUILD_64Bit)
    if (BUILD_64Bit)
        set(TEMP_NAME "${TEMP_NAME}-64")
    else()
        set(TEMP_NAME "${TEMP_NAME}-32")
    endif()
    set(TEMP_NAME "${TEMP_NAME}-${MAJOR_VERSION}")
    set_target_properties(${LIB_NAME} PROPERTIES OUTPUT_NAME ${TEMP_NAME})

    if (WIN32 AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX))
        set_target_properties(${LIB_NAME} PROPERTIES PREFIX "")
    endif()
endfunction()

function(opendaq_prepend_path PATH FILES)
  foreach(SOURCE_FILE ${${FILES}})
    set(MODIFIED ${MODIFIED} "${PATH}/${SOURCE_FILE}")
  endforeach()

  set(${FILES} ${MODIFIED} PARENT_SCOPE)
endfunction()
