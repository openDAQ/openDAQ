set(LIB_NAME ${SDK_TARGET_NAME}_utils)
set(INCLUDE_PREFIX ${SDK_TARGET_NAME}/utils)

list(APPEND CMAKE_MESSAGE_CONTEXT ${LIB_NAME})

set(SOURCE_CPPS function_thread.cpp
                thread_ex.cpp
                timer_thread.cpp
                thread_name.cpp
)

set(SOURCE_HEADERS finally.h
                   function_thread.h
                   utils.h
                   thread_ex.h
                   timer_thread.h
                   thread_name.h
)

opendaq_prepend_include(${INCLUDE_PREFIX} SOURCE_HEADERS)

add_library(${LIB_NAME} STATIC ${SOURCE_CPPS} ${SOURCE_HEADERS})
add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_include_directories(${LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS ${LIB_NAME}
        EXPORT ${SDK_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${SOURCE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${INCLUDE_PREFIX}
)

