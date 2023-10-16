set(MODULE_NAME ${SDK_TARGET_NAME}_utils)
list(APPEND CMAKE_MESSAGE_CONTEXT ${MODULE_NAME})

set(SOURCE_CPPS function_thread.cpp
                thread_ex.cpp
                timer_thread.cpp
)

set(SOURCE_HEADERS finally.h
                   function_thread.h
                   utils.h
                   thread_ex.h
                   timer_thread.h
)

prepend_include(${SDK_TARGET_NAME}/utils SOURCE_HEADERS)

add_library(${MODULE_NAME} STATIC ${SOURCE_CPPS} ${SOURCE_HEADERS})
add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

        $<INSTALL_INTERFACE:include>
)
